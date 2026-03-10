#include "quickcontrolservice.h"
#include "database/databasemanager.h"
#include "services/deviceservice.h"
#include "services/sceneservice.h" // 假设你有这个
#include "database/dao/SceneDao.h"
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QHash>

namespace
{
    QString iconForDevice(const QString &deviceType, const QString &deviceName, const QString &deviceId)
    {
        const QString type = deviceType.toLower();
        const QString name = deviceName.toLower();
        const QString id = deviceId.toLower();

        if (type.contains(QStringLiteral("照明")))
            return QString(":/icons/light.svg");
        if (type.contains(QStringLiteral("空调")) || type.contains(QStringLiteral("温控")))
            return QString(":/icons/ac.svg");
        if (type.contains(QStringLiteral("窗帘")) || type.contains(QStringLiteral("遮阳")))
            return QString(":/icons/curtains.svg");
        if (type.contains(QStringLiteral("锁")) || name.contains(QStringLiteral("锁")) || id.contains("lock"))
            return QString(":/icons/lock.svg");
        if (type.contains(QStringLiteral("摄像")) || name.contains(QStringLiteral("摄像")) || id.contains("camera"))
            return QString(":/icons/check.svg");
        if (type.contains(QStringLiteral("影音")) || type.contains(QStringLiteral("电视")) || name.contains(QStringLiteral("电视")) || id.contains("tv"))
            return QString(":/icons/tv.svg");
        return QString(":/icons/devices.svg");
    }

    bool isTurnOnAction(const QString &actionText)
    {
        const QString trimmed = actionText.trimmed();
        const QString lower = trimmed.toLower();
        return trimmed == QStringLiteral("开启") || trimmed == QStringLiteral("打开") || trimmed == QStringLiteral("解锁") || lower == "on" || lower == "open" || lower == "unlock" || lower == "true";
    }

    int parseParamValue(const QString &paramText)
    {
        QString digits;
        for (const QChar ch : paramText)
        {
            if (ch.isDigit() || (ch == '-' && digits.isEmpty()))
            {
                digits.append(ch);
            }
        }
        return digits.toInt();
    }
}

QList<QuickControlDisplayItem> QuickControlService::getHomeShortcuts() const
{
    QList<QuickControlDisplayItem> result;
    DatabaseManager &db = DatabaseManager::instance();

    // 优化：使用 UNION ALL 和 JOIN 一次性查出设备和场景的完整数据，解决 N+1 查询性能问题
    QString sql = R"(
        SELECT 
            q.id AS mapping_id, 
            q.target_type, 
            q.target_id, 
            q.display_order,
            d.device_id AS string_id, 
            d.device_name AS display_name, 
            d.device_type AS device_type,
            d.online_status, 
            d.switch_status,
            '' AS icon_path
        FROM quick_controls q
        INNER JOIN devices d ON q.target_id = d.id AND q.target_type = 'device'
        UNION ALL
        SELECT 
            q.id AS mapping_id, 
            q.target_type, 
            q.target_id, 
            q.display_order,
            s.scene_code AS string_id, 
            s.scene_name AS display_name, 
            '' AS device_type,
            'online' AS online_status, 
            'on' AS switch_status,
            s.scene_icon AS icon_path
        FROM quick_controls q
        INNER JOIN scenes s ON q.target_id = s.id AND q.target_type = 'scene'
        ORDER BY display_order ASC
    )";

    QSqlQuery query = db.query(sql, {});
    while (query.next())
    {
        QuickControlDisplayItem item;
        item.mappingId = query.value("mapping_id").toLongLong();
        item.targetType = query.value("target_type").toString();
        item.targetId = query.value("target_id").toLongLong();
        item.targetStringId = query.value("string_id").toString();
        item.displayName = query.value("display_name").toString();

        // 状态解析
        QString onlineStatus = query.value("online_status").toString();
        QString switchStatus = query.value("switch_status").toString();
        item.isOnline = (onlineStatus == "online");
        item.isOn = (switchStatus == "on");

        // 图标处理（可根据设备类型补全，这里做基础判断）
        item.iconPath = query.value("icon_path").toString();
        if (item.targetType == "device" && item.iconPath.isEmpty())
        {
            item.iconPath = iconForDevice(query.value("device_type").toString(),
                                          item.displayName,
                                          item.targetStringId);
        }

        result.push_back(item);
    }

    return result;
}

bool QuickControlService::addShortcut(const QString &targetType, long long targetId, QString *errorMessage) const
{
    QuickControlDao dao;
    if (!dao.addShortcut(targetType, targetId))
    {
        if (errorMessage)
            *errorMessage = dao.lastErrorText();
        return false;
    }
    return true;
}

bool QuickControlService::removeShortcut(const QString &targetType, long long targetId, QString *errorMessage) const
{
    QuickControlDao dao;
    if (!dao.removeShortcut(targetType, targetId))
    {
        if (errorMessage)
            *errorMessage = dao.lastErrorText();
        return false;
    }
    return true;
}

bool QuickControlService::executeShortcut(const QuickControlDisplayItem &item, bool targetState, QString *errorMessage) const
{
    // 门面模式 (Facade)：向 UI 屏蔽底层的 DeviceService 和 SceneService 差异
    if (item.targetType == "device")
    {
        DeviceService deviceService;
        return deviceService.updateSwitchState(item.targetStringId, targetState, errorMessage);
    }
    else if (item.targetType == "scene")
    {
        SceneDao sceneDao;
        const SceneList scenes = sceneDao.listScenesWithActions();

        SceneDefinition targetScene;
        bool found = false;
        for (const SceneDefinition &scene : scenes)
        {
            if (scene.id == item.targetStringId)
            {
                targetScene = scene;
                found = true;
                break;
            }
        }

        if (!found)
        {
            if (errorMessage)
                *errorMessage = QStringLiteral("未找到要触发的场景");
            return false;
        }

        DeviceService deviceService;
        const DeviceList devices = deviceService.loadDefaultDevices();
        QHash<QString, DeviceDefinition> deviceIndex;
        for (const DeviceDefinition &device : devices)
        {
            deviceIndex.insert(device.id, device);
        }

        for (const SceneDeviceAction &action : targetScene.actions)
        {
            QString actionError;
            bool ok = false;
            if (!action.paramText.trimmed().isEmpty() && deviceIndex.contains(action.deviceId))
            {
                DeviceDefinition targetDevice = deviceIndex.value(action.deviceId);
                targetDevice.value = parseParamValue(action.paramText);
                ok = deviceService.updateDeviceValue(targetDevice, targetDevice.value, &actionError);
            }
            else
            {
                ok = deviceService.updateSwitchState(action.deviceId, isTurnOnAction(action.actionText), &actionError);
            }

            if (!ok)
            {
                if (errorMessage)
                {
                    *errorMessage = QStringLiteral("场景执行失败：") + action.deviceName + QStringLiteral("，") + actionError;
                }
                return false;
            }
        }

        qInfo() << "[QuickControl] 场景执行完成:" << targetScene.name;
        Q_UNUSED(targetState);
        return true;
    }

    if (errorMessage)
        *errorMessage = "未知的快捷控制类型";
    return false;
}

QList<QuickControlManageItem> QuickControlService::getAllManageableItems() const
{
    QList<QuickControlManageItem> result;
    DatabaseManager &db = DatabaseManager::instance();

    // 一次性查出所有的设备和场景，并用子查询判断它是否在 quick_controls 表中
    QString sql = R"(
        SELECT 
            'device' AS target_type, 
            id AS target_id, 
            device_name AS display_name,
            (SELECT COUNT(*) FROM quick_controls q WHERE q.target_type = 'device' AND q.target_id = devices.id) > 0 AS is_selected
        FROM devices
        UNION ALL
        SELECT 
            'scene' AS target_type, 
            id AS target_id, 
            scene_name AS display_name,
            (SELECT COUNT(*) FROM quick_controls q WHERE q.target_type = 'scene' AND q.target_id = scenes.id) > 0 AS is_selected
        FROM scenes
    )";

    QSqlQuery query = db.query(sql, {});
    while (query.next())
    {
        QuickControlManageItem item;
        item.targetType = query.value("target_type").toString();
        item.targetId = query.value("target_id").toLongLong();
        item.displayName = query.value("display_name").toString();
        item.isSelected = query.value("is_selected").toBool();
        result.push_back(item);
    }
    return result;
}

bool QuickControlService::saveManageableItems(const QList<QuickControlManageItem> &items, QString *errorMessage) const
{
    QuickControlDao dao;
    DatabaseManager &db = DatabaseManager::instance();

    if (!db.beginTransaction())
    {
        if (errorMessage)
            *errorMessage = db.lastErrorText();
        return false;
    }

    // 覆盖式更新：先清空，再把勾选的重新插入
    if (!dao.clearAll())
    {
        db.rollback();
        if (errorMessage)
            *errorMessage = dao.lastErrorText();
        return false;
    }

    for (const auto &item : items)
    {
        if (item.isSelected)
        {
            if (!dao.addShortcut(item.targetType, item.targetId))
            {
                db.rollback();
                if (errorMessage)
                    *errorMessage = dao.lastErrorText();
                return false;
            }
        }
    }

    if (!db.commit())
    {
        db.rollback();
        if (errorMessage)
            *errorMessage = db.lastErrorText();
        return false;
    }

    return true;
}