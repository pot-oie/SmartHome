#include "SystemConfigDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QSqlQuery>
#include <QVariant>

namespace
{
const char *LOG_PREFIX = "[SystemConfigDao]";
}

AlarmThreshold SystemConfigDao::getAlarmThresholds()
{
    AlarmThreshold threshold{18.0, 30.0, 30.0, 70.0};

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before threshold query:" << m_lastErrorText;
        return threshold;
    }

    if (!ensureDefaultConfigRow())
    {
        return threshold;
    }

    static const QString sql =
        "SELECT temperature_low_threshold, temperature_high_threshold, "
        "humidity_low_threshold, humidity_high_threshold "
        "FROM system_configs WHERE config_name = ? LIMIT 1";

    QSqlQuery query = databaseManager.query(sql, {QStringLiteral("default")});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Threshold query failed:" << m_lastErrorText;
        return threshold;
    }

    if (!query.next())
    {
        clearLastError();
        return threshold;
    }

    clearLastError();
    threshold.tempMin = query.value("temperature_low_threshold").toDouble();
    threshold.tempMax = query.value("temperature_high_threshold").toDouble();
    threshold.humidityMin = query.value("humidity_low_threshold").toDouble();
    threshold.humidityMax = query.value("humidity_high_threshold").toDouble();
    return threshold;
}

bool SystemConfigDao::saveAlarmThresholds(const AlarmThreshold &threshold, qint64 updatedBy)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before threshold save:" << m_lastErrorText;
        return false;
    }

    static const QString sql =
        "INSERT INTO system_configs ("
        "config_name, temperature_low_threshold, temperature_high_threshold, "
        "humidity_low_threshold, humidity_high_threshold, "
        "system_status_level, system_status_text, updated_by"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE "
        "temperature_low_threshold = VALUES(temperature_low_threshold), "
        "temperature_high_threshold = VALUES(temperature_high_threshold), "
        "humidity_low_threshold = VALUES(humidity_low_threshold), "
        "humidity_high_threshold = VALUES(humidity_high_threshold), "
        "updated_by = VALUES(updated_by), "
        "updated_at = CURRENT_TIMESTAMP";

    const QVariant updatedByValue = updatedBy > 0
                                        ? QVariant::fromValue(updatedBy)
                                        : QVariant(QVariant::LongLong);
    const bool ok = databaseManager.exec(
        sql,
        {QStringLiteral("default"),
         threshold.tempMin,
         threshold.tempMax,
         threshold.humidityMin,
         threshold.humidityMax,
         QStringLiteral("normal"),
         QStringLiteral("系统正常，无报警"),
         updatedByValue});
    if (!ok)
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Threshold save failed:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    return true;
}

bool SystemConfigDao::updateAlarmStatus(const AlarmStatusSummary &status)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before alarm status save:" << m_lastErrorText;
        return false;
    }

    static const QString sql =
        "INSERT INTO system_configs (config_name, system_status_level, system_status_text) "
        "VALUES (?, ?, ?) "
        "ON DUPLICATE KEY UPDATE "
        "system_status_level = VALUES(system_status_level), "
        "system_status_text = VALUES(system_status_text), "
        "updated_at = CURRENT_TIMESTAMP";

    const bool ok = databaseManager.exec(
        sql,
        {QStringLiteral("default"),
         status.level.isEmpty() ? QStringLiteral("normal") : status.level,
         status.text.isEmpty() ? QStringLiteral("系统正常，无报警") : status.text});
    if (!ok)
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Alarm status save failed:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    return true;
}

AlarmStatusSummary SystemConfigDao::getAlarmStatusSummary()
{
    AlarmStatusSummary summary;
    summary.level = QStringLiteral("normal");
    summary.text = QStringLiteral("系统正常，无报警");

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before system status query:" << m_lastErrorText;
        return summary;
    }

    if (!ensureDefaultConfigRow())
    {
        return summary;
    }

    static const QString sql =
        "SELECT system_status_level, system_status_text "
        "FROM system_configs WHERE config_name = ? LIMIT 1";

    QSqlQuery query = databaseManager.query(sql, {QStringLiteral("default")});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "System status query failed:" << m_lastErrorText;
        return summary;
    }

    if (!query.next())
    {
        clearLastError();
        return summary;
    }

    clearLastError();
    summary.level = query.value("system_status_level").toString().trimmed();
    summary.text = query.value("system_status_text").toString().trimmed();
    if (summary.level.isEmpty())
    {
        summary.level = QStringLiteral("normal");
    }
    if (summary.text.isEmpty())
    {
        summary.text = QStringLiteral("系统正常，无报警");
    }
    return summary;
}

QString SystemConfigDao::getSystemStatusText()
{
    return getAlarmStatusSummary().text;
}

QString SystemConfigDao::lastErrorText() const
{
    return m_lastErrorText;
}

bool SystemConfigDao::ensureDefaultConfigRow()
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    static const QString sql =
        "INSERT INTO system_configs ("
        "config_name, temperature_low_threshold, temperature_high_threshold, "
        "humidity_low_threshold, humidity_high_threshold, "
        "system_status_level, system_status_text"
        ") VALUES (?, ?, ?, ?, ?, ?, ?) "
        "ON DUPLICATE KEY UPDATE config_name = VALUES(config_name)";

    const bool ok = databaseManager.exec(
        sql,
        {QStringLiteral("default"),
         18.0,
         30.0,
         30.0,
         70.0,
         QStringLiteral("normal"),
         QStringLiteral("系统正常，无报警")});
    if (!ok)
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Ensure default system config failed:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    return true;
}

void SystemConfigDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void SystemConfigDao::clearLastError()
{
    m_lastErrorText.clear();
}
