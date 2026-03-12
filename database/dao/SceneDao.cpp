#include "SceneDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QHash>
#include <QSqlQuery>

namespace
{
    const char *LOG_PREFIX = "[SceneDao]";

    bool hasSceneIconColumn(DatabaseManager &databaseManager)
    {
        static const QString sql = "SHOW COLUMNS FROM scenes LIKE 'scene_icon'";
        QSqlQuery query = databaseManager.query(sql, {});
        return query.isActive() && query.next();
    }

    QString matchSceneIconByCode(const QString &sceneCode)
    {
        const QString code = sceneCode.trimmed().toLower();
        if (code.contains("home"))
        {
            return ":/icons/home.svg";
        }
        if (code.contains("sleep"))
        {
            return ":/icons/bedtime.svg";
        }
        if (code.contains("movie"))
        {
            return ":/icons/movie.svg";
        }
        if (code.contains("away") || code.contains("leave"))
        {
            return ":/icons/flight_takeoff.svg";
        }
        if (code.contains("party"))
        {
            return ":/icons/celebration.svg";
        }
        if (code.contains("wakeup") || code.contains("morning"))
        {
            return ":/icons/wb_sunny.svg";
        }
        return ":/icons/scene.svg";
    }

    bool resolveDevicePrimaryKey(DatabaseManager &databaseManager, const SceneDeviceAction &action, qlonglong *devicePk, QString *errorText)
    {
        const QString deviceUniqueId = action.deviceId.trimmed();
        QSqlQuery deviceQuery;

        if (!deviceUniqueId.isEmpty())
        {
            deviceQuery = databaseManager.query("SELECT id FROM devices WHERE device_id = ? LIMIT 1", {deviceUniqueId});
            if (deviceQuery.isActive() && deviceQuery.next())
            {
                *devicePk = deviceQuery.value(0).toLongLong();
                return true;
            }
        }

        const QString deviceName = action.deviceName.trimmed();
        if (!deviceName.isEmpty())
        {
            deviceQuery = databaseManager.query("SELECT id FROM devices WHERE device_name = ? LIMIT 1", {deviceName});
            if (deviceQuery.isActive() && deviceQuery.next())
            {
                *devicePk = deviceQuery.value(0).toLongLong();
                return true;
            }
        }

        if (errorText)
        {
            *errorText = QString::fromUtf8("未找到对应设备，请先在系统设置中维护设备信息。");
        }
        return false;
    }

    qlonglong findSceneActionRecordId(DatabaseManager &databaseManager, const QString &sceneCode, const SceneDeviceAction &action)
    {
        if (action.recordId > 0)
        {
            return action.recordId;
        }

        static const QString sql =
            "SELECT a.id FROM scene_actions a "
            "INNER JOIN scenes s ON s.id = a.scene_id "
            "INNER JOIN devices d ON d.id = a.device_id "
            "WHERE s.scene_code = ? AND d.device_name = ? AND a.action_name = ? "
            "AND COALESCE(NULLIF(TRIM(a.action_param), ''), NULLIF(TRIM(a.param_value), ''), '') = COALESCE(?, '') "
            "ORDER BY a.action_order ASC, a.id ASC LIMIT 1";

        QSqlQuery query = databaseManager.query(sql, {sceneCode, action.deviceName, action.actionText, action.paramText});
        if (!query.isActive() || !query.next())
        {
            return 0;
        }

        return query.value(0).toLongLong();
    }
}

SceneList SceneDao::listScenesWithActions()
{
    SceneList scenes;

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before scene query:" << m_lastErrorText;
        return scenes;
    }

    const bool iconColumnExists = hasSceneIconColumn(databaseManager);
    const QString sceneSql = iconColumnExists
                                 ? "SELECT scene_code, scene_name, scene_description, scene_icon "
                                   "FROM scenes WHERE is_enabled = 1 ORDER BY display_order ASC, id ASC"
                                 : "SELECT scene_code, scene_name, scene_description "
                                   "FROM scenes WHERE is_enabled = 1 ORDER BY display_order ASC, id ASC";

    QSqlQuery sceneQuery = databaseManager.query(sceneSql, {});
    if (!sceneQuery.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Scene list query failed:" << m_lastErrorText;
        return scenes;
    }

    QHash<QString, int> sceneIndexByCode;
    while (sceneQuery.next())
    {
        SceneDefinition scene;
        scene.id = sceneQuery.value("scene_code").toString();
        scene.name = sceneQuery.value("scene_name").toString();
        scene.description = sceneQuery.value("scene_description").toString();
        scene.icon = iconColumnExists ? sceneQuery.value("scene_icon").toString().trimmed() : QString();
        if (scene.icon.isEmpty())
        {
            scene.icon = matchSceneIconByCode(scene.id);
        }
        sceneIndexByCode.insert(scene.id, scenes.size());
        scenes.push_back(scene);
    }

    static const QString actionSql =
        "SELECT a.id AS action_id, s.scene_code, d.device_id, d.device_name, a.action_name, "
        "COALESCE(NULLIF(TRIM(a.action_param), ''), NULLIF(TRIM(a.param_value), ''), '') AS action_param "
        "FROM scene_actions a "
        "INNER JOIN scenes s ON s.id = a.scene_id "
        "INNER JOIN devices d ON d.id = a.device_id "
        "WHERE s.is_enabled = 1 "
        "ORDER BY s.display_order ASC, a.action_order ASC, a.id ASC";

    QSqlQuery actionQuery = databaseManager.query(actionSql, {});
    if (!actionQuery.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Scene action query failed:" << m_lastErrorText;
        return scenes;
    }

    while (actionQuery.next())
    {
        const QString sceneCode = actionQuery.value("scene_code").toString();
        if (!sceneIndexByCode.contains(sceneCode))
        {
            continue;
        }

        SceneDeviceAction action;
        action.recordId = actionQuery.value("action_id").toLongLong();
        action.deviceId = actionQuery.value("device_id").toString();
        action.deviceName = actionQuery.value("device_name").toString();
        action.actionText = actionQuery.value("action_name").toString();
        action.paramText = actionQuery.value("action_param").toString();

        scenes[sceneIndexByCode.value(sceneCode)].actions.push_back(action);
    }

    clearLastError();
    return scenes;
}

bool SceneDao::insertScene(SceneDefinition &scene)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before scene insert:" << m_lastErrorText;
        return false;
    }

    static const QString nextOrderSql = "SELECT COALESCE(MAX(display_order), 0) + 1 FROM scenes";
    QSqlQuery nextOrderQuery = databaseManager.query(nextOrderSql, {});
    if (!nextOrderQuery.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Query next scene order failed:" << m_lastErrorText;
        return false;
    }

    int nextOrder = 1;
    if (nextOrderQuery.next())
    {
        nextOrder = nextOrderQuery.value(0).toInt();
        if (nextOrder <= 0)
        {
            nextOrder = 1;
        }
    }

    const bool iconColumnExists = hasSceneIconColumn(databaseManager);
    const QString iconPath = scene.icon.trimmed().isEmpty() ? QString(":/icons/scene.svg") : scene.icon.trimmed();
    const bool hasPresetCode = !scene.id.trimmed().isEmpty();
    const QString insertSql = hasPresetCode
                                  ? (iconColumnExists
                                         ? "INSERT INTO scenes (scene_code, scene_name, scene_description, scene_icon, is_default, is_enabled, display_order) "
                                           "VALUES (?, ?, ?, ?, 0, 1, ?)"
                                         : "INSERT INTO scenes (scene_code, scene_name, scene_description, is_default, is_enabled, display_order) "
                                           "VALUES (?, ?, ?, 0, 1, ?)")
                                  : (iconColumnExists
                                         ? "INSERT INTO scenes (scene_code, scene_name, scene_description, scene_icon, is_default, is_enabled, display_order) "
                                           "VALUES (CONCAT('scene_', UUID_SHORT()), ?, ?, ?, 0, 1, ?)"
                                         : "INSERT INTO scenes (scene_code, scene_name, scene_description, is_default, is_enabled, display_order) "
                                           "VALUES (CONCAT('scene_', UUID_SHORT()), ?, ?, 0, 1, ?)");

    const QVariantList params = hasPresetCode
                                    ? (iconColumnExists
                                           ? QVariantList{scene.id.trimmed(), scene.name, scene.description, iconPath, nextOrder}
                                           : QVariantList{scene.id.trimmed(), scene.name, scene.description, nextOrder})
                                    : (iconColumnExists
                                           ? QVariantList{scene.name, scene.description, iconPath, nextOrder}
                                           : QVariantList{scene.name, scene.description, nextOrder});

    if (!databaseManager.exec(insertSql, params))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Insert scene failed:" << m_lastErrorText << "| scene_name:" << scene.name;
        return false;
    }

    if (hasPresetCode)
    {
        scene.id = scene.id.trimmed();
        clearLastError();
        return true;
    }

    QSqlQuery codeQuery = databaseManager.query("SELECT scene_code FROM scenes WHERE id = LAST_INSERT_ID() LIMIT 1", {});
    if (!codeQuery.isActive() || !codeQuery.next())
    {
        setLastError(databaseManager.lastErrorText().isEmpty()
                         ? QString::fromUtf8("新增场景后读取 scene_code 失败。")
                         : databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << m_lastErrorText;
        return false;
    }

    scene.id = codeQuery.value(0).toString().trimmed();

    clearLastError();
    return true;
}

bool SceneDao::updateScene(const SceneDefinition &scene)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before scene update:" << m_lastErrorText;
        return false;
    }

    const bool iconColumnExists = hasSceneIconColumn(databaseManager);
    const QString iconPath = scene.icon.trimmed().isEmpty() ? QString(":/icons/scene.svg") : scene.icon.trimmed();
    const QString updateSql = iconColumnExists
                                  ? "UPDATE scenes SET scene_name = ?, scene_description = ?, scene_icon = ? WHERE scene_code = ?"
                                  : "UPDATE scenes SET scene_name = ?, scene_description = ? WHERE scene_code = ?";

    const QVariantList params = iconColumnExists
                                    ? QVariantList{scene.name, scene.description, iconPath, scene.id}
                                    : QVariantList{scene.name, scene.description, scene.id};

    if (!databaseManager.exec(updateSql, params))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Update scene failed:" << m_lastErrorText << "| scene_code:" << scene.id;
        return false;
    }

    clearLastError();
    return true;
}

bool SceneDao::insertSceneAction(const QString &sceneCode, const SceneDeviceAction &action)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before scene action insert:" << m_lastErrorText;
        return false;
    }

    qlonglong devicePk = 0;
    QString resolveError;
    if (!resolveDevicePrimaryKey(databaseManager, action, &devicePk, &resolveError))
    {
        setLastError(resolveError);
        return false;
    }

    static const QString insertActionSql =
        "INSERT INTO scene_actions (scene_id, device_id, action_name, action_param, param_value, action_order) "
        "SELECT s.id, ?, ?, ?, ?, COALESCE((SELECT MAX(action_order) FROM scene_actions WHERE scene_id = s.id), 0) + 1 "
        "FROM scenes s WHERE s.scene_code = ? LIMIT 1";

    if (!databaseManager.exec(insertActionSql, {devicePk, action.actionText, action.paramText, action.paramText, sceneCode}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Insert scene action failed:" << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| device:" << action.deviceName;
        return false;
    }

    clearLastError();
    return true;
}

bool SceneDao::updateSceneAction(const QString &sceneCode, const SceneDeviceAction &oldAction, const SceneDeviceAction &newAction)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before scene action update:" << m_lastErrorText;
        return false;
    }

    const qlonglong actionId = findSceneActionRecordId(databaseManager, sceneCode, oldAction);
    if (actionId <= 0)
    {
        setLastError(QString::fromUtf8("未找到可更新的场景设备动作记录。"));
        qWarning().noquote() << LOG_PREFIX << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| device:" << oldAction.deviceName;
        return false;
    }

    qlonglong devicePk = 0;
    QString resolveError;
    if (!resolveDevicePrimaryKey(databaseManager, newAction, &devicePk, &resolveError))
    {
        setLastError(resolveError);
        return false;
    }

    static const QString updateSql =
        "UPDATE scene_actions SET device_id = ?, action_name = ?, action_param = ?, param_value = ? "
        "WHERE id = ? AND scene_id = (SELECT id FROM scenes WHERE scene_code = ? LIMIT 1)";

    if (!databaseManager.exec(updateSql, {devicePk, newAction.actionText, newAction.paramText, newAction.paramText, actionId, sceneCode}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Update scene action failed:" << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| action_id:" << actionId;
        return false;
    }

    clearLastError();
    return true;
}

bool SceneDao::deleteSceneAction(const QString &sceneCode, const SceneDeviceAction &action)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before scene action delete:" << m_lastErrorText;
        return false;
    }

    const qlonglong actionId = findSceneActionRecordId(databaseManager, sceneCode, action);
    if (actionId <= 0)
    {
        setLastError(QString::fromUtf8("未找到可删除的场景设备动作记录。"));
        qWarning().noquote() << LOG_PREFIX << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| device:" << action.deviceName;
        return false;
    }

    static const QString deleteSql = "DELETE FROM scene_actions WHERE id = ?";
    if (!databaseManager.exec(deleteSql, {actionId}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Delete scene action failed:" << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| device:" << action.deviceName;
        return false;
    }

    clearLastError();
    return true;
}

bool SceneDao::deleteSceneByCode(const QString &sceneCode)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before scene delete:" << m_lastErrorText;
        return false;
    }

    static const QString sql = "DELETE FROM scenes WHERE scene_code = ?";
    if (!databaseManager.exec(sql, {sceneCode}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Delete scene failed:" << m_lastErrorText << "| scene_code:" << sceneCode;
        return false;
    }

    clearLastError();
    return true;
}

QString SceneDao::lastErrorText() const
{
    return m_lastErrorText;
}

void SceneDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void SceneDao::clearLastError()
{
    m_lastErrorText.clear();
}
