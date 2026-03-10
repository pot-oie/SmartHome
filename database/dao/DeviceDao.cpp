#include "DeviceDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QUuid>

namespace
{
    const char *LOG_PREFIX = "[DeviceDao]";

    struct DeviceSeedCategory
    {
        const char *code;
        const char *name;
        int order;
    };

    struct DeviceSeedRow
    {
        const char *deviceId;
        const char *deviceName;
        const char *deviceType;
        const char *ipAddress;
        const char *onlineStatus;
        const char *switchStatus;
        double currentValue;
        const char *valueUnit;
        bool supportsSlider;
        double minValue;
        double maxValue;
        int displayOrder;
        bool homeQuickControl;
        const char *quickControlKey;
        const char *remarks;
    };

    QList<DeviceSeedCategory> defaultCategories()
    {
        return {
            {"light", "\u7167\u660e\u8bbe\u5907", 1},
            {"air_conditioner", "\u7a7a\u8c03\u8bbe\u5907", 2},
            {"curtain", "\u7a97\u5e18\u8bbe\u5907", 3},
            {"security", "\u5b89\u9632\u8bbe\u5907", 4},
            {"media", "\u5f71\u97f3\u8bbe\u5907", 5},
            {"sensor", "\u4f20\u611f\u8bbe\u5907", 6}};
    }

    QList<DeviceSeedRow> defaultDevices()
    {
        return {
            {"light_living", "\u5ba2\u5385\u4e3b\u706f", "\u7167\u660e\u8bbe\u5907", "192.168.1.101", "online", "on", 80.0, "%", true, 0.0, 100.0, 1, true, "btnQuickLight", "\u5ba2\u5385\u4e3b\u706f"},
            {"light_bedroom", "\u5367\u5ba4\u706f", "\u7167\u660e\u8bbe\u5907", "192.168.1.102", "online", "off", 60.0, "%", true, 0.0, 100.0, 2, false, nullptr, "\u5367\u5ba4\u7167\u660e"},
            {"light_kitchen", "\u53a8\u623f\u706f", "\u7167\u660e\u8bbe\u5907", "192.168.1.103", "online", "on", 100.0, "%", true, 0.0, 100.0, 3, false, nullptr, "\u53a8\u623f\u7167\u660e"},
            {"ac_living", "\u5ba2\u5385\u7a7a\u8c03", "\u7a7a\u8c03\u8bbe\u5907", "192.168.1.104", "online", "on", 24.0, "C", true, 16.0, 30.0, 4, true, "btnQuickAC", "\u5ba2\u5385\u7a7a\u8c03"},
            {"ac_bedroom", "\u5367\u5ba4\u7a7a\u8c03", "\u7a7a\u8c03\u8bbe\u5907", "192.168.1.105", "online", "off", 26.0, "C", true, 16.0, 30.0, 5, false, nullptr, "\u5367\u5ba4\u7a7a\u8c03"},
            {"curtain_living", "\u5ba2\u5385\u7a97\u5e18", "\u7a97\u5e18\u8bbe\u5907", "192.168.1.106", "online", "on", 100.0, "%", true, 0.0, 100.0, 6, true, "btnQuickCurtain", "\u5ba2\u5385\u7a97\u5e18"},
            {"curtain_bedroom", "\u5367\u5ba4\u7a97\u5e18", "\u7a97\u5e18\u8bbe\u5907", "192.168.1.107", "offline", "off", 0.0, "%", true, 0.0, 100.0, 7, false, nullptr, "\u5367\u5ba4\u7a97\u5e18"},
            {"lock_door", "\u524d\u95e8\u667a\u80fd\u9501", "\u5b89\u9632\u8bbe\u5907", "192.168.1.108", "online", "on", 1.0, "", false, 0.0, 1.0, 8, true, "btnQuickDoor", "\u5165\u6237\u95e8\u9501"},
            {"camera_01", "\u5ba2\u5385\u6444\u50cf\u5934", "\u5b89\u9632\u8bbe\u5907", "192.168.1.109", "online", "on", 0.0, "", false, 0.0, 0.0, 9, false, nullptr, "\u5ba2\u5385\u6444\u50cf\u5934"},
            {"tv_living", "\u5ba2\u5385\u7535\u89c6", "\u5f71\u97f3\u8bbe\u5907", "192.168.1.110", "online", "off", 50.0, "%", true, 0.0, 100.0, 10, true, "btnQuickTV", "\u5ba2\u5385\u7535\u89c6"}};
    }

    QString iconForType(const QString &deviceType)
    {
        if (deviceType.contains(QStringLiteral("\u7167\u660e")))
        {
            return QString(":/icons/light.svg");
        }
        if (deviceType.contains(QStringLiteral("\u7a7a\u8c03")) || deviceType.contains(QStringLiteral("\u6e29\u63a7")))
        {
            return QString(":/icons/ac.svg");
        }
        if (deviceType.contains(QStringLiteral("\u7a97\u5e18")))
        {
            return QString(":/icons/curtains.svg");
        }
        if (deviceType.contains(QStringLiteral("\u9501")))
        {
            return QString(":/icons/lock.svg");
        }
        if (deviceType.contains(QStringLiteral("\u5b89\u9632")))
        {
            return QString(":/icons/check.svg");
        }
        if (deviceType.contains(QStringLiteral("\u5f71\u97f3")) || deviceType.contains(QStringLiteral("\u7535\u89c6")))
        {
            return QString(":/icons/tv.svg");
        }
        return QString(":/icons/devices.svg");
    }

    QString defaultCategoryCodeForType(const QString &deviceType)
    {
        if (deviceType.contains(QStringLiteral("\u7167\u660e")))
        {
            return QString("light");
        }
        if (deviceType.contains(QStringLiteral("\u7a7a\u8c03")) || deviceType.contains(QStringLiteral("\u6e29\u63a7")))
        {
            return QString("air_conditioner");
        }
        if (deviceType.contains(QStringLiteral("\u7a97\u5e18")) || deviceType.contains(QStringLiteral("\u906e\u9633")))
        {
            return QString("curtain");
        }
        if (deviceType.contains(QStringLiteral("\u5b89\u9632")) || deviceType.contains(QStringLiteral("\u9501")) || deviceType.contains(QStringLiteral("\u6444\u50cf")))
        {
            return QString("security");
        }
        if (deviceType.contains(QStringLiteral("\u5f71\u97f3")) || deviceType.contains(QStringLiteral("\u7535\u89c6")))
        {
            return QString("media");
        }
        if (deviceType.contains(QStringLiteral("\u4f20\u611f")))
        {
            return QString("sensor");
        }

        QString code = deviceType.toLower();
        code.remove(QRegularExpression("[^a-z0-9]+"));
        if (!code.isEmpty())
        {
            return code;
        }

        return QString("custom_%1").arg(QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(8));
    }

    bool resolveCategoryId(DatabaseManager &databaseManager, const QString &deviceType, qlonglong *categoryId, QString *errorText)
    {
        QSqlQuery query = databaseManager.query("SELECT id FROM device_categories WHERE category_name = ? LIMIT 1", {deviceType});
        if (query.isActive() && query.next())
        {
            *categoryId = query.value(0).toLongLong();
            return true;
        }

        const QString categoryCode = defaultCategoryCodeForType(deviceType);
        query = databaseManager.query("SELECT id FROM device_categories WHERE category_code = ? LIMIT 1", {categoryCode});
        if (query.isActive() && query.next())
        {
            *categoryId = query.value(0).toLongLong();
            return true;
        }

        QSqlQuery orderQuery = databaseManager.query("SELECT COALESCE(MAX(display_order), 0) + 1 FROM device_categories", {});
        int nextOrder = 1;
        if (orderQuery.isActive() && orderQuery.next())
        {
            nextOrder = qMax(1, orderQuery.value(0).toInt());
        }

        if (!databaseManager.exec(
                "INSERT INTO device_categories (category_code, category_name, display_order, is_enabled) VALUES (?, ?, ?, 1)",
                {categoryCode, deviceType, nextOrder}))
        {
            if (errorText)
            {
                *errorText = databaseManager.lastErrorText();
            }
            return false;
        }

        query = databaseManager.query("SELECT id FROM device_categories WHERE category_code = ? LIMIT 1", {categoryCode});
        if (!query.isActive() || !query.next())
        {
            if (errorText)
            {
                *errorText = databaseManager.lastErrorText().isEmpty()
                                 ? QStringLiteral("\u65b0\u589e\u8bbe\u5907\u5206\u7c7b\u540e\u8bfb\u53d6\u5206\u7c7b\u4e3b\u952e\u5931\u8d25\u3002")
                                 : databaseManager.lastErrorText();
            }
            return false;
        }

        *categoryId = query.value(0).toLongLong();
        return true;
    }

    QString valueUnitForType(const QString &deviceType)
    {
        if (deviceType.contains(QStringLiteral("\u7a7a\u8c03")) || deviceType.contains(QStringLiteral("\u6e29\u63a7")))
        {
            return QString("C");
        }
        if (deviceType.contains(QStringLiteral("\u7167\u660e")) || deviceType.contains(QStringLiteral("\u7a97\u5e18")) || deviceType.contains(QStringLiteral("\u5f71\u97f3")))
        {
            return QString("%");
        }
        return QString();
    }

    bool supportsSliderForType(const QString &deviceType)
    {
        return deviceType.contains(QStringLiteral("\u7167\u660e"))
               || deviceType.contains(QStringLiteral("\u7a7a\u8c03"))
               || deviceType.contains(QStringLiteral("\u6e29\u63a7"))
               || deviceType.contains(QStringLiteral("\u7a97\u5e18"))
               || deviceType.contains(QStringLiteral("\u5f71\u97f3"));
    }

    QPair<double, double> sliderRangeForType(const QString &deviceType)
    {
        if (deviceType.contains(QStringLiteral("\u7a7a\u8c03")) || deviceType.contains(QStringLiteral("\u6e29\u63a7")))
        {
            return {16.0, 30.0};
        }
        return {0.0, 100.0};
    }

    QString modeTextForState(const QString &deviceType, const QString &switchStatus, double currentValue)
    {
        if (switchStatus == "off")
        {
            return QStringLiteral("\u5173\u95ed");
        }

        if (deviceType.contains(QStringLiteral("\u7a7a\u8c03")))
        {
            return QStringLiteral("\u8bbe\u5b9a %1C").arg(qRound(currentValue));
        }
        if (deviceType.contains(QStringLiteral("\u7167\u660e")))
        {
            return QStringLiteral("\u4eae\u5ea6 %1%").arg(qRound(currentValue));
        }
        if (deviceType.contains(QStringLiteral("\u7a97\u5e18")))
        {
            return QStringLiteral("\u5f00\u542f %1%").arg(qRound(currentValue));
        }
        if (deviceType.contains(QStringLiteral("\u5b89\u9632")) && currentValue > 0.0)
        {
            return QStringLiteral("\u5df2\u5f00\u542f");
        }
        return QStringLiteral("\u5f00\u542f");
    }
}

int DeviceDao::countAllDevices()
{
    return executeCountQuery("SELECT COUNT(*) FROM devices");
}

int DeviceDao::countOnlineDevices()
{
    return executeCountQuery("SELECT COUNT(*) FROM devices WHERE online_status = ?", {"online"});
}

QList<DeviceDao::DeviceCategoryRow> DeviceDao::listEnabledCategories()
{
    QList<DeviceCategoryRow> categories;

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before category query:" << m_lastErrorText;
        return categories;
    }

    static const QString sql =
        "SELECT category_code, category_name, display_order "
        "FROM device_categories WHERE is_enabled = 1 ORDER BY display_order ASC, id ASC";

    QSqlQuery query = databaseManager.query(sql, {});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Category query failed:" << m_lastErrorText;
        return categories;
    }

    while (query.next())
    {
        DeviceCategoryRow row;
        row.code = query.value("category_code").toString();
        row.name = query.value("category_name").toString();
        row.order = query.value("display_order").toInt();
        categories.push_back(row);
    }

    clearLastError();
    return categories;
}

DeviceList DeviceDao::listDeviceDefinitions()
{
    DeviceList devices;

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before device definition query:" << m_lastErrorText;
        return devices;
    }

    static const QString sql =
        "SELECT d.device_id, d.device_name, c.category_name, d.online_status, d.switch_status, d.current_value, "
        "d.value_unit, d.supports_slider, d.slider_min, d.slider_max "
        "FROM devices d "
        "INNER JOIN device_categories c ON c.id = d.category_id "
        "ORDER BY d.display_order ASC, d.id ASC";

    QSqlQuery query = databaseManager.query(sql, {});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Device definition query failed:" << m_lastErrorText;
        return devices;
    }

    while (query.next())
    {
        DeviceDefinition device;
        device.id = query.value("device_id").toString();
        device.name = query.value("device_name").toString();
        device.type = query.value("category_name").toString();
        device.icon = iconForType(device.type);
        device.isOnline = (query.value("online_status").toString() == "online");
        device.isOn = (query.value("switch_status").toString() == "on");
        device.value = qRound(query.value("current_value").toDouble());
        device.valueUnit = query.value("value_unit").toString();
        device.supportsSlider = query.value("supports_slider").toInt() == 1;
        device.minValue = qRound(query.value("slider_min").toDouble());
        device.maxValue = qRound(query.value("slider_max").toDouble());
        if (device.supportsSlider && device.maxValue <= device.minValue)
        {
            const QPair<double, double> range = sliderRangeForType(device.type);
            device.minValue = qRound(range.first);
            device.maxValue = qRound(range.second);
        }
        devices.push_back(device);
    }

    clearLastError();
    return devices;
}

SettingsDeviceList DeviceDao::listSettingsDevices()
{
    SettingsDeviceList devices;

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before settings device query:" << m_lastErrorText;
        return devices;
    }

    static const QString sql =
        "SELECT device_id, device_name, device_type, ip_address, online_status "
        "FROM devices ORDER BY display_order ASC, id ASC";

    QSqlQuery query = databaseManager.query(sql, {});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Settings device query failed:" << m_lastErrorText;
        return devices;
    }

    while (query.next())
    {
        SettingsDeviceEntry device;
        device.id = query.value("device_id").toString();
        device.name = query.value("device_name").toString();
        device.type = query.value("device_type").toString();
        device.ip = query.value("ip_address").toString();
        device.online = (query.value("online_status").toString() == "online");
        devices.push_back(device);
    }

    clearLastError();
    return devices;
}

bool DeviceDao::ensureDefaultDeviceData()
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before seed default devices:" << m_lastErrorText;
        return false;
    }

    if (!databaseManager.beginTransaction())
    {
        setLastError(databaseManager.lastErrorText());
        return false;
    }

    for (const DeviceSeedCategory &category : defaultCategories())
    {
        if (!databaseManager.exec(
                "INSERT INTO device_categories (category_code, category_name, display_order, is_enabled) "
                "VALUES (?, ?, ?, 1) "
                "ON DUPLICATE KEY UPDATE category_name = VALUES(category_name), display_order = VALUES(display_order), is_enabled = 1",
                {QString::fromUtf8(category.code), QString::fromUtf8(category.name), category.order}))
        {
            setLastError(databaseManager.lastErrorText());
            databaseManager.rollback();
            return false;
        }
    }

    for (const DeviceSeedRow &seed : defaultDevices())
    {
        qlonglong categoryId = 0;
        QString errorText;
        if (!resolveCategoryId(databaseManager, QString::fromUtf8(seed.deviceType), &categoryId, &errorText))
        {
            setLastError(errorText);
            databaseManager.rollback();
            return false;
        }

        if (!databaseManager.exec(
                "INSERT INTO devices ("
                "device_id, category_id, device_name, device_type, ip_address, online_status, switch_status, "
                "current_value, value_unit, supports_slider, slider_min, slider_max, display_order, "
                "is_home_quick_control, quick_control_key, remarks, last_seen_at, last_control_at"
                ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NOW(), NOW()) "
                "ON DUPLICATE KEY UPDATE "
                "category_id = VALUES(category_id), device_name = VALUES(device_name), device_type = VALUES(device_type), "
                "ip_address = VALUES(ip_address), online_status = VALUES(online_status), switch_status = VALUES(switch_status), "
                "current_value = VALUES(current_value), value_unit = VALUES(value_unit), supports_slider = VALUES(supports_slider), "
                "slider_min = VALUES(slider_min), slider_max = VALUES(slider_max), display_order = VALUES(display_order), "
                "is_home_quick_control = VALUES(is_home_quick_control), quick_control_key = VALUES(quick_control_key), "
                "remarks = VALUES(remarks), last_seen_at = NOW(), last_control_at = NOW()",
                {QString::fromUtf8(seed.deviceId),
                 categoryId,
                 QString::fromUtf8(seed.deviceName),
                 QString::fromUtf8(seed.deviceType),
                 QString::fromUtf8(seed.ipAddress),
                 QString::fromUtf8(seed.onlineStatus),
                 QString::fromUtf8(seed.switchStatus),
                 seed.currentValue,
                 QString::fromUtf8(seed.valueUnit),
                 seed.supportsSlider ? 1 : 0,
                 seed.supportsSlider ? QVariant(seed.minValue) : QVariant(),
                 seed.supportsSlider ? QVariant(seed.maxValue) : QVariant(),
                 seed.displayOrder,
                 seed.homeQuickControl ? 1 : 0,
                 seed.quickControlKey ? QVariant(QString::fromUtf8(seed.quickControlKey)) : QVariant(),
                 QString::fromUtf8(seed.remarks)}))
        {
            setLastError(databaseManager.lastErrorText());
            databaseManager.rollback();
            return false;
        }

        QSqlQuery deviceQuery = databaseManager.query("SELECT id FROM devices WHERE device_id = ? LIMIT 1", {QString::fromUtf8(seed.deviceId)});
        if (!deviceQuery.isActive() || !deviceQuery.next())
        {
            setLastError(databaseManager.lastErrorText().isEmpty()
                             ? QStringLiteral("\u5199\u5165\u9ed8\u8ba4\u8bbe\u5907\u540e\u8bfb\u53d6\u8bbe\u5907\u4e3b\u952e\u5931\u8d25\u3002")
                             : databaseManager.lastErrorText());
            databaseManager.rollback();
            return false;
        }

        const qlonglong devicePk = deviceQuery.value(0).toLongLong();
        const QString deviceType = QString::fromUtf8(seed.deviceType);
        const QString switchStatus = QString::fromUtf8(seed.switchStatus);
        const QString modeText = modeTextForState(deviceType, switchStatus, seed.currentValue);
        if (!databaseManager.exec(
                "INSERT INTO device_state_snapshots (device_id, online_status, switch_status, current_value, value_unit, mode_text, last_reported_at) "
                "VALUES (?, ?, ?, ?, ?, ?, NOW()) "
                "ON DUPLICATE KEY UPDATE online_status = VALUES(online_status), switch_status = VALUES(switch_status), "
                "current_value = VALUES(current_value), value_unit = VALUES(value_unit), mode_text = VALUES(mode_text), "
                "last_reported_at = NOW()",
                {devicePk,
                 QString::fromUtf8(seed.onlineStatus),
                 switchStatus,
                 seed.currentValue,
                 QString::fromUtf8(seed.valueUnit),
                 modeText}))
        {
            setLastError(databaseManager.lastErrorText());
            databaseManager.rollback();
            return false;
        }
    }

    if (!databaseManager.commit())
    {
        setLastError(databaseManager.lastErrorText());
        databaseManager.rollback();
        return false;
    }

    clearLastError();
    return true;
}

bool DeviceDao::insertDevice(const SettingsDeviceEntry &device)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before insert device:" << m_lastErrorText;
        return false;
    }

    qlonglong categoryId = 0;
    QString errorText;
    if (!resolveCategoryId(databaseManager, device.type, &categoryId, &errorText))
    {
        setLastError(errorText);
        return false;
    }

    const bool supportsSlider = supportsSliderForType(device.type);
    const QPair<double, double> range = sliderRangeForType(device.type);
    const QString valueUnit = valueUnitForType(device.type);
    const double initialValue = supportsSlider ? range.first : (device.online ? 1.0 : 0.0);
    const int nextOrder = executeCountQuery("SELECT COUNT(*) FROM devices") + 1;

    if (!databaseManager.beginTransaction())
    {
        setLastError(databaseManager.lastErrorText());
        return false;
    }

    const QVariantList deviceParams = {
        device.id,
        categoryId,
        device.name,
        device.type,
        device.ip,
        device.online ? "online" : "offline",
        device.online ? "on" : "off",
        initialValue,
        valueUnit,
        supportsSlider ? 1 : 0,
        supportsSlider ? QVariant(range.first) : QVariant(),
        supportsSlider ? QVariant(range.second) : QVariant(),
        nextOrder,
        QStringLiteral("\u65b0\u589e\u8bbe\u5907")};

    if (!databaseManager.exec(
            "INSERT INTO devices ("
            "device_id, category_id, device_name, device_type, ip_address, online_status, switch_status, "
            "current_value, value_unit, supports_slider, slider_min, slider_max, display_order, remarks"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)",
            deviceParams))
    {
        setLastError(databaseManager.lastErrorText());
        databaseManager.rollback();
        qWarning().noquote() << LOG_PREFIX << "Insert device failed:" << m_lastErrorText << "| device_id:" << device.id;
        return false;
    }

    QSqlQuery deviceQuery = databaseManager.query("SELECT id FROM devices WHERE device_id = ? LIMIT 1", {device.id});
    if (!deviceQuery.isActive() || !deviceQuery.next())
    {
        setLastError(databaseManager.lastErrorText().isEmpty()
                         ? QStringLiteral("\u65b0\u589e\u8bbe\u5907\u540e\u8bfb\u53d6\u8bbe\u5907\u4e3b\u952e\u5931\u8d25\u3002")
                         : databaseManager.lastErrorText());
        databaseManager.rollback();
        return false;
    }

    const qlonglong devicePk = deviceQuery.value(0).toLongLong();
    if (!databaseManager.exec(
            "INSERT INTO device_state_snapshots (device_id, online_status, switch_status, current_value, value_unit, mode_text, last_reported_at) "
            "VALUES (?, ?, ?, ?, ?, ?, NOW())",
            {devicePk,
             device.online ? "online" : "offline",
             device.online ? "on" : "off",
             initialValue,
             valueUnit,
             modeTextForState(device.type, device.online ? "on" : "off", initialValue)}))
    {
        setLastError(databaseManager.lastErrorText());
        databaseManager.rollback();
        return false;
    }

    if (!databaseManager.commit())
    {
        setLastError(databaseManager.lastErrorText());
        databaseManager.rollback();
        return false;
    }

    clearLastError();
    return true;
}

bool DeviceDao::updateDeviceState(const QString &deviceId,
                                  const QString &onlineStatus,
                                  const QString &switchStatus,
                                  double currentValue,
                                  const QString &valueUnit,
                                  const QString &modeText)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before update device state:" << m_lastErrorText;
        return false;
    }

    QSqlQuery deviceQuery = databaseManager.query("SELECT id, device_type FROM devices WHERE device_id = ? LIMIT 1", {deviceId});
    if (!deviceQuery.isActive() || !deviceQuery.next())
    {
        setLastError(databaseManager.lastErrorText().isEmpty()
                         ? QStringLiteral("\u672a\u627e\u5230\u8981\u66f4\u65b0\u7684\u8bbe\u5907\u3002")
                         : databaseManager.lastErrorText());
        return false;
    }

    const qlonglong devicePk = deviceQuery.value("id").toLongLong();
    const QString deviceType = deviceQuery.value("device_type").toString();
    const QString finalUnit = valueUnit.isNull() ? valueUnitForType(deviceType) : valueUnit;
    const QString finalModeText = modeText.trimmed().isEmpty() ? modeTextForState(deviceType, switchStatus, currentValue) : modeText.trimmed();

    if (!databaseManager.beginTransaction())
    {
        setLastError(databaseManager.lastErrorText());
        return false;
    }

    if (!databaseManager.exec(
            "UPDATE devices "
            "SET online_status = ?, switch_status = ?, current_value = ?, value_unit = ?, last_seen_at = NOW(), last_control_at = NOW() "
            "WHERE device_id = ?",
            {onlineStatus, switchStatus, currentValue, finalUnit, deviceId}))
    {
        setLastError(databaseManager.lastErrorText());
        databaseManager.rollback();
        return false;
    }

    if (!databaseManager.exec(
            "INSERT INTO device_state_snapshots (device_id, online_status, switch_status, current_value, value_unit, mode_text, last_reported_at) "
            "VALUES (?, ?, ?, ?, ?, ?, NOW()) "
            "ON DUPLICATE KEY UPDATE online_status = VALUES(online_status), switch_status = VALUES(switch_status), "
            "current_value = VALUES(current_value), value_unit = VALUES(value_unit), mode_text = VALUES(mode_text), "
            "last_reported_at = NOW()",
            {devicePk, onlineStatus, switchStatus, currentValue, finalUnit, finalModeText}))
    {
        setLastError(databaseManager.lastErrorText());
        databaseManager.rollback();
        return false;
    }

    if (!databaseManager.commit())
    {
        setLastError(databaseManager.lastErrorText());
        databaseManager.rollback();
        return false;
    }

    clearLastError();
    return true;
}

bool DeviceDao::deleteDeviceById(const QString &deviceId)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before delete device:" << m_lastErrorText;
        return false;
    }

    static const QString sql = "DELETE FROM devices WHERE device_id = ?";
    if (!databaseManager.exec(sql, {deviceId}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Delete device failed:" << m_lastErrorText << "| device_id:" << deviceId;
        return false;
    }

    clearLastError();
    return true;
}

QString DeviceDao::lastErrorText() const
{
    return m_lastErrorText;
}

int DeviceDao::executeCountQuery(const QString &sql, const QVariantList &params)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before count query:" << m_lastErrorText;
        return 0;
    }

    QSqlQuery query = databaseManager.query(sql, params);
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Count query failed:" << m_lastErrorText << "| SQL:" << sql;
        return 0;
    }

    if (!query.next())
    {
        clearLastError();
        qInfo().noquote() << LOG_PREFIX << "Count query returned no rows. Fallback to 0.";
        return 0;
    }

    clearLastError();
    return query.value(0).toInt();
}

void DeviceDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void DeviceDao::clearLastError()
{
    m_lastErrorText.clear();
}
