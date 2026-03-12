#include "DeviceDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QMetaType>
#include <QRegularExpression>
#include <QSqlQuery>
#include <QUuid>

namespace
{
    const char *LOG_PREFIX = "[DeviceDao]";

    QString iconForType(const QString &deviceType, const QString &deviceName, const QString &deviceId)
    {
        if (deviceType.contains(QStringLiteral("\u7167\u660e")))
        {
            return QString(":/icons/light.svg");
        }
        if (deviceType.contains(QStringLiteral("\u7a7a\u8c03")) || deviceType.contains(QStringLiteral("\u6e29\u63a7")))
        {
            return QString(":/icons/ac.svg");
        }
        const QString type = deviceType.toLower();
        const QString name = deviceName.toLower();
        const QString id = deviceId.toLower();

        if (type.contains(QStringLiteral("锁")) || name.contains(QStringLiteral("锁")) || id.contains("lock"))
        {
            return QString(":/icons/lock.svg");
        }
        if (type.contains(QStringLiteral("摄像")) || name.contains(QStringLiteral("摄像")) || id.contains("camera"))
        {
            return QString(":/icons/check.svg");
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
        return deviceType.contains(QStringLiteral("\u7167\u660e")) || deviceType.contains(QStringLiteral("\u7a7a\u8c03")) || deviceType.contains(QStringLiteral("\u6e29\u63a7")) || deviceType.contains(QStringLiteral("\u7a97\u5e18")) || deviceType.contains(QStringLiteral("\u5f71\u97f3"));
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
    return executeCountQuery(
        "SELECT COUNT(*) "
        "FROM devices d "
        "LEFT JOIN device_state_snapshots s ON s.device_id = d.id "
        "WHERE COALESCE(s.online_status, d.online_status) = ?",
        {"online"});
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
        "SELECT d.device_id, d.device_name, c.category_name, "
        "COALESCE(s.online_status, d.online_status) AS effective_online_status, "
        "COALESCE(s.switch_status, d.switch_status) AS effective_switch_status, "
        "COALESCE(s.current_value, d.current_value) AS effective_current_value, "
        "COALESCE(NULLIF(s.value_unit, ''), d.value_unit) AS effective_value_unit, "
        "d.supports_slider, d.slider_min, d.slider_max "
        "FROM devices d "
        "LEFT JOIN device_state_snapshots s ON s.device_id = d.id "
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
        device.icon = iconForType(device.type, device.name, device.id);
        device.isOnline = (query.value("effective_online_status").toString() == "online");
        device.isOn = (query.value("effective_switch_status").toString() == "on");
        device.value = qRound(query.value("effective_current_value").toDouble());
        device.valueUnit = query.value("effective_value_unit").toString();
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
        "SELECT d.device_id, d.device_name, d.device_type, d.ip_address, "
        "COALESCE(s.online_status, d.online_status) AS effective_online_status "
        "FROM devices d "
        "LEFT JOIN device_state_snapshots s ON s.device_id = d.id "
        "ORDER BY d.display_order ASC, d.id ASC";

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
        device.onlineStatus = query.value("effective_online_status").toString().trimmed().toLower();
        devices.push_back(device);
    }

    clearLastError();
    return devices;
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
    const bool isOnline = (device.onlineStatus == QStringLiteral("online"));
    const double initialValue = supportsSlider ? range.first : (isOnline ? 1.0 : 0.0);
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
        isOnline ? "online" : "offline",
        isOnline ? "on" : "off",
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
             isOnline ? "online" : "offline",
             isOnline ? "on" : "off",
             initialValue,
             valueUnit,
             modeTextForState(device.type, isOnline ? "on" : "off", initialValue)}))
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

QVariantMap DeviceDao::loadDeviceExtraParams(const QString &deviceId)
{
    QVariantMap params;

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before loading extra params:" << m_lastErrorText;
        return params;
    }

    static const QString sql =
        "SELECT p.param_code, p.param_type, p.param_value_int, p.param_value_decimal, p.param_value_text "
        "FROM device_control_params p "
        "INNER JOIN devices d ON d.id = p.device_id "
        "WHERE d.device_id = ? AND p.is_enabled = 1";

    QSqlQuery query = databaseManager.query(sql, {deviceId});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Load extra params query failed:" << m_lastErrorText << "| device_id:" << deviceId;
        return params;
    }

    while (query.next())
    {
        const QString code = query.value("param_code").toString().trimmed();
        if (code.isEmpty())
        {
            continue;
        }

        const QString type = query.value("param_type").toString().trimmed().toLower();
        if (type == QStringLiteral("int") || type == QStringLiteral("bool"))
        {
            params.insert(code, query.value("param_value_int").toInt());
        }
        else if (type == QStringLiteral("decimal"))
        {
            params.insert(code, query.value("param_value_decimal").toDouble());
        }
        else
        {
            params.insert(code, query.value("param_value_text").toString());
        }
    }

    clearLastError();
    return params;
}

QHash<QString, QVariantMap> DeviceDao::loadDeviceExtraParamsBatch(const QStringList &deviceIds)
{
    QHash<QString, QVariantMap> allParams;
    if (deviceIds.isEmpty())
    {
        return allParams;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before batch loading extra params:" << m_lastErrorText;
        return allParams;
    }

    QStringList placeholders;
    QVariantList params;
    placeholders.reserve(deviceIds.size());
    params.reserve(deviceIds.size());
    for (const QString &deviceId : deviceIds)
    {
        placeholders.push_back(QStringLiteral("?"));
        params.push_back(deviceId);
    }

    const QString sql = QStringLiteral(
                            "SELECT d.device_id AS device_key, p.param_code, p.param_type, "
                            "p.param_value_int, p.param_value_decimal, p.param_value_text "
                            "FROM device_control_params p "
                            "INNER JOIN devices d ON d.id = p.device_id "
                            "WHERE p.is_enabled = 1 AND d.device_id IN (%1)")
                            .arg(placeholders.join(QStringLiteral(",")));

    QSqlQuery query = databaseManager.query(sql, params);
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Batch load extra params query failed:" << m_lastErrorText;
        return allParams;
    }

    while (query.next())
    {
        const QString deviceKey = query.value("device_key").toString().trimmed();
        const QString code = query.value("param_code").toString().trimmed();
        if (deviceKey.isEmpty() || code.isEmpty())
        {
            continue;
        }

        const QString type = query.value("param_type").toString().trimmed().toLower();
        QVariant value;
        if (type == QStringLiteral("int") || type == QStringLiteral("bool"))
        {
            value = query.value("param_value_int").toInt();
        }
        else if (type == QStringLiteral("decimal"))
        {
            value = query.value("param_value_decimal").toDouble();
        }
        else
        {
            value = query.value("param_value_text").toString();
        }

        QVariantMap perDevice = allParams.value(deviceKey);
        perDevice.insert(code, value);
        allParams.insert(deviceKey, perDevice);
    }

    clearLastError();
    return allParams;
}

bool DeviceDao::upsertDeviceExtraParam(const QString &deviceId,
                                       const QString &paramCode,
                                       const QVariant &paramValue,
                                       const QString &paramName,
                                       const QString &paramUnit)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before upsert extra param:" << m_lastErrorText;
        return false;
    }

    QSqlQuery deviceQuery = databaseManager.query("SELECT id FROM devices WHERE device_id = ? LIMIT 1", {deviceId});
    if (!deviceQuery.isActive() || !deviceQuery.next())
    {
        setLastError(databaseManager.lastErrorText().isEmpty()
                         ? QStringLiteral("未找到要更新参数的设备。")
                         : databaseManager.lastErrorText());
        return false;
    }

    const qlonglong devicePk = deviceQuery.value(0).toLongLong();
    const QString finalCode = paramCode.trimmed();
    if (finalCode.isEmpty())
    {
        setLastError(QStringLiteral("参数编码不能为空。"));
        return false;
    }

    QString paramType = QStringLiteral("text");
    QVariant valueInt;
    QVariant valueDecimal;
    QVariant valueText;
    if (paramValue.userType() == QMetaType::Int
        || paramValue.userType() == QMetaType::UInt
        || paramValue.userType() == QMetaType::LongLong
        || paramValue.userType() == QMetaType::ULongLong
        || paramValue.userType() == QMetaType::Bool)
    {
        paramType = (paramValue.userType() == QMetaType::Bool) ? QStringLiteral("bool") : QStringLiteral("int");
        valueInt = paramValue.toInt();
        valueText = QString::number(paramValue.toInt());
    }
    else if (paramValue.userType() == QMetaType::Double || paramValue.userType() == QMetaType::Float)
    {
        paramType = QStringLiteral("decimal");
        valueDecimal = paramValue.toDouble();
        valueText = QString::number(paramValue.toDouble(), 'f', 2);
    }
    else
    {
        valueText = paramValue.toString();
    }

    if (!databaseManager.exec(
            "INSERT INTO device_control_params ("
            "device_id, param_code, param_name, param_type, param_value_int, param_value_decimal, param_value_text, param_unit, is_realtime, is_enabled, updated_at"
            ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, 1, 1, NOW()) "
            "ON DUPLICATE KEY UPDATE param_name = VALUES(param_name), param_type = VALUES(param_type), "
            "param_value_int = VALUES(param_value_int), param_value_decimal = VALUES(param_value_decimal), "
            "param_value_text = VALUES(param_value_text), param_unit = VALUES(param_unit), is_realtime = 1, is_enabled = 1, updated_at = NOW()",
            {devicePk,
             finalCode,
             paramName.trimmed().isEmpty() ? finalCode : paramName,
             paramType,
             valueInt,
             valueDecimal,
             valueText,
             paramUnit}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Upsert extra param failed:" << m_lastErrorText << "| device_id:" << deviceId << "| param:" << finalCode;
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
