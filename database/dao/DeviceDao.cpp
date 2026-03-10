#include "DeviceDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QSqlQuery>
#include <QUuid>

namespace
{
    const char *LOG_PREFIX = "[DeviceDao]";
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
        "SELECT d.device_id, d.device_name, c.category_name, d.online_status, d.switch_status, d.current_value "
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
        device.icon = ":/icons/devices.svg";
        device.isOnline = (query.value("online_status").toString() == "online");
        device.isOn = (query.value("switch_status").toString() == "on");
        device.value = qRound(query.value("current_value").toDouble());
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
        const QString onlineStatus = query.value("online_status").toString();
        device.online = (onlineStatus == "online");
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

    static const QString sql =
        "INSERT INTO devices ("
        "device_id, category_id, device_name, device_type, ip_address, online_status, switch_status, "
        "current_value, value_unit, supports_slider, slider_min, slider_max, display_order, remarks"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";

    const int nextOrder = executeCountQuery("SELECT COUNT(*) FROM devices") + 1;
    const QVariantList params = {
        device.id,
        6,
        device.name,
        device.type,
        device.ip,
        device.online ? "online" : "offline",
        "off",
        0,
        QVariant(),
        0,
        QVariant(),
        QVariant(),
        nextOrder,
        "新增设备"};

    if (!databaseManager.exec(sql, params))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Insert device failed:" << m_lastErrorText << "| device_id:" << device.id;
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
