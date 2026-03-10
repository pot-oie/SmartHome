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
                                   "FROM scenes ORDER BY display_order ASC, id ASC"
                                 : "SELECT scene_code, scene_name, scene_description "
                                   "FROM scenes ORDER BY display_order ASC, id ASC";

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
        "SELECT s.scene_code, d.device_id, d.device_name, a.action_name, a.action_param "
        "FROM scene_actions a "
        "INNER JOIN scenes s ON s.id = a.scene_id "
        "INNER JOIN devices d ON d.id = a.device_id "
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
    const QString insertSql = iconColumnExists
                                  ? "INSERT INTO scenes (scene_code, scene_name, scene_description, scene_icon, is_default, display_order) "
                                    "VALUES (CONCAT('scene_', UUID_SHORT()), ?, ?, ?, 0, ?)"
                                  : "INSERT INTO scenes (scene_code, scene_name, scene_description, is_default, display_order) "
                                    "VALUES (CONCAT('scene_', UUID_SHORT()), ?, ?, 0, ?)";

    const QVariantList params = iconColumnExists
                                    ? QVariantList{scene.name, scene.description, iconPath, nextOrder}
                                    : QVariantList{scene.name, scene.description, nextOrder};

    if (!databaseManager.exec(insertSql, params))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Insert scene failed:" << m_lastErrorText << "| scene_name:" << scene.name;
        return false;
    }

    QSqlQuery codeQuery = databaseManager.query("SELECT scene_code FROM scenes WHERE id = LAST_INSERT_ID() LIMIT 1", {});
    if (!codeQuery.isActive() || !codeQuery.next())
    {
        setLastError(databaseManager.lastErrorText().isEmpty() ? QString("新增场景后读取 scene_code 失败。") : databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << m_lastErrorText;
        return false;
    }

    scene.id = codeQuery.value(0).toString().trimmed();

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

    static const QString findDeviceSql =
        "SELECT id FROM devices WHERE device_name = ? LIMIT 1";
    QSqlQuery deviceQuery = databaseManager.query(findDeviceSql, {action.deviceName});
    if (!deviceQuery.isActive() || !deviceQuery.next())
    {
        setLastError("未找到对应设备，请先在系统设置中维护该设备。");
        return false;
    }

    const qlonglong devicePk = deviceQuery.value(0).toLongLong();

    static const QString insertActionSql =
        "INSERT INTO scene_actions (scene_id, device_id, action_name, action_param, action_order) "
        "SELECT s.id, ?, ?, ?, COALESCE((SELECT MAX(action_order) FROM scene_actions WHERE scene_id = s.id), 0) + 1 "
        "FROM scenes s WHERE s.scene_code = ? LIMIT 1";

    if (!databaseManager.exec(insertActionSql, {devicePk, action.actionText, action.paramText, sceneCode}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Insert scene action failed:" << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| device:" << action.deviceName;
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

    static const QString findActionSql =
        "SELECT a.id FROM scene_actions a "
        "INNER JOIN scenes s ON s.id = a.scene_id "
        "INNER JOIN devices d ON d.id = a.device_id "
        "WHERE s.scene_code = ? AND d.device_name = ? AND a.action_name = ? AND COALESCE(a.action_param, '') = COALESCE(?, '') "
        "ORDER BY a.action_order ASC, a.id ASC LIMIT 1";

    QSqlQuery actionQuery = databaseManager.query(findActionSql, {sceneCode, action.deviceName, action.actionText, action.paramText});
    if (!actionQuery.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Find scene action failed:" << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| device:" << action.deviceName;
        return false;
    }

    if (!actionQuery.next())
    {
        setLastError("未找到可删除的设备动作记录。");
        qWarning().noquote() << LOG_PREFIX << m_lastErrorText
                             << "| scene_code:" << sceneCode << "| device:" << action.deviceName;
        return false;
    }

    const qlonglong actionId = actionQuery.value(0).toLongLong();
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
