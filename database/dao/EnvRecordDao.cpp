#include "EnvRecordDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QSqlQuery>

namespace
{
const char *LOG_PREFIX = "[EnvRecordDao]";
}

std::optional<QPair<double, double>> EnvRecordDao::getLatestTemperatureAndHumidity()
{
    const std::optional<EnvRealtimeSnapshot> snapshot = getLatestRealtimeSnapshot();
    if (!snapshot.has_value())
    {
        return std::nullopt;
    }

    return QPair<double, double>(snapshot->temperature, snapshot->humidity);
}

std::optional<EnvRealtimeSnapshot> EnvRecordDao::getLatestRealtimeSnapshot()
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before env query:" << m_lastErrorText;
        return std::nullopt;
    }

    static const QString sql =
        "SELECT id, location_code, location_name, source_device_id, "
        "temperature, humidity, pm25, co2, status_level, updated_at "
        "FROM env_realtime_snapshots "
        "ORDER BY updated_at DESC LIMIT 1";

    QSqlQuery query = databaseManager.query(sql, {});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Latest env query failed:" << m_lastErrorText;
        return std::nullopt;
    }

    if (!query.next())
    {
        clearLastError();
        qInfo().noquote() << LOG_PREFIX << "No env record found.";
        return std::nullopt;
    }

    EnvRealtimeSnapshot snapshot;
    snapshot.recordId = query.value("id").toLongLong();
    snapshot.locationCode = query.value("location_code").toString().trimmed();
    snapshot.locationName = query.value("location_name").toString().trimmed();
    snapshot.sourceDeviceId = query.value("source_device_id").toLongLong();
    snapshot.temperature = query.value("temperature").toDouble();
    snapshot.humidity = query.value("humidity").toDouble();
    snapshot.pm25 = query.value("pm25").toDouble();
    snapshot.co2 = query.value("co2").toDouble();
    snapshot.statusLevel = query.value("status_level").toString().trimmed();
    snapshot.updatedAt = query.value("updated_at").toDateTime();

    clearLastError();
    return snapshot;
}

QString EnvRecordDao::lastErrorText() const
{
    return m_lastErrorText;
}

void EnvRecordDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void EnvRecordDao::clearLastError()
{
    m_lastErrorText.clear();
}
