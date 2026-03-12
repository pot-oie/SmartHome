#include "AlarmDao.h"

#include "../databasemanager.h"

#include <QDateTime>
#include <QDebug>
#include <QLocale>
#include <QSqlQuery>
#include <QVariant>

namespace
{
const char *LOG_PREFIX = "[AlarmDao]";

QString composeTriggerText(const QSqlQuery &query)
{
    const QString displayText = query.value("trigger_display_text").toString().trimmed();
    const QString unit = query.value("trigger_unit").toString().trimmed();
    if (!displayText.isEmpty())
    {
        if (!unit.isEmpty() && !displayText.endsWith(unit))
        {
            return displayText + unit;
        }
        return displayText;
    }

    const QVariant rawValue = query.value("trigger_value_decimal");
    if (!rawValue.isValid() || rawValue.isNull())
    {
        return QStringLiteral("-");
    }

    return QLocale().toString(rawValue.toDouble(), 'f', 2) + unit;
}

QString composeTriggerText(double triggerValue, const QString &displayText, const QString &unit)
{
    if (!displayText.trimmed().isEmpty())
    {
        if (!unit.trimmed().isEmpty() && !displayText.endsWith(unit))
        {
            return displayText + unit;
        }
        return displayText;
    }

    return QLocale().toString(triggerValue, 'f', 2) + unit;
}

QString levelFromSeverity(const QString &severity)
{
    if (severity == QStringLiteral("critical"))
    {
        return QStringLiteral("critical");
    }
    if (severity == QStringLiteral("warning"))
    {
        return QStringLiteral("warning");
    }
    return QStringLiteral("normal");
}
}

QList<QString> AlarmDao::getRecentAlarmTexts(int limit)
{
    QList<QString> alarmTexts;
    const int safeLimit = qBound(1, limit, 20);

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before recent alarm query:" << m_lastErrorText;
        return alarmTexts;
    }

    static const QString sql =
        "SELECT created_at, alarm_type, alarm_content "
        "FROM alarm_records ORDER BY created_at DESC LIMIT ?";

    QSqlQuery query = databaseManager.query(sql, {safeLimit});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Recent alarm query failed:" << m_lastErrorText;
        return alarmTexts;
    }

    while (query.next())
    {
        const QDateTime createdAt = query.value("created_at").toDateTime();
        const QString alarmType = query.value("alarm_type").toString().trimmed();
        const QString alarmContent = query.value("alarm_content").toString().trimmed();

        const QString line = QStringLiteral("[%1] %2 | %3")
                                 .arg(createdAt.toString("yyyy-MM-dd HH:mm:ss"))
                                 .arg(alarmType.isEmpty() ? QStringLiteral("Alarm") : alarmType)
                                 .arg(alarmContent.isEmpty() ? QStringLiteral("No details") : alarmContent);
        alarmTexts.append(line);
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "Loaded recent alarms:" << alarmTexts.size();
    return alarmTexts;
}

AlarmLogList AlarmDao::listAlarmLogs(int limit)
{
    AlarmLogList logs;
    const int safeLimit = qBound(1, limit, 500);

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before alarm log query:" << m_lastErrorText;
        return logs;
    }

    static const QString sql =
        "SELECT id, created_at, alarm_type, alarm_content, severity, "
        "trigger_value_decimal, trigger_display_text, trigger_unit, "
        "handled_status, is_active "
        "FROM alarm_records "
        "ORDER BY created_at DESC LIMIT ?";

    QSqlQuery query = databaseManager.query(sql, {safeLimit});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Alarm log query failed:" << m_lastErrorText;
        return logs;
    }

    while (query.next())
    {
        AlarmLogEntry entry;
        entry.recordId = query.value("id").toLongLong();
        entry.timestamp = query.value("created_at").toDateTime();
        entry.type = query.value("alarm_type").toString().trimmed();
        entry.triggerValue = composeTriggerText(query);
        entry.detail = query.value("alarm_content").toString().trimmed();
        entry.severity = query.value("severity").toString().trimmed();
        entry.handledStatus = query.value("handled_status").toString().trimmed();
        entry.isActive = query.value("is_active").toBool();
        logs.push_back(entry);
    }

    clearLastError();
    return logs;
}

AlarmStatusSummary AlarmDao::getActiveAlarmStatusSummary()
{
    AlarmStatusSummary summary;

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before active alarm query:" << m_lastErrorText;
        return summary;
    }

    static const QString countSql =
        "SELECT COUNT(*) AS active_count "
        "FROM alarm_records WHERE is_active = 1";
    QSqlQuery countQuery = databaseManager.query(countSql);
    if (!countQuery.isActive() || !countQuery.next())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Active alarm count query failed:" << m_lastErrorText;
        return summary;
    }

    summary.activeCount = countQuery.value("active_count").toInt();
    if (summary.activeCount <= 0)
    {
        clearLastError();
        return summary;
    }

    static const QString detailSql =
        "SELECT alarm_type, alarm_content, severity, created_at "
        "FROM alarm_records WHERE is_active = 1 "
        "ORDER BY created_at DESC LIMIT 1";
    QSqlQuery detailQuery = databaseManager.query(detailSql);
    if (!detailQuery.isActive() || !detailQuery.next())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Active alarm detail query failed:" << m_lastErrorText;
        return summary;
    }

    const QString severity = detailQuery.value("severity").toString().trimmed();
    const QString alarmContent = detailQuery.value("alarm_content").toString().trimmed();
    const QString alarmType = detailQuery.value("alarm_type").toString().trimmed();
    const QString latestText = alarmContent.isEmpty() ? alarmType : alarmContent;

    summary.level = levelFromSeverity(severity);
    summary.text = summary.activeCount == 1
                       ? QStringLiteral("当前有 1 条活动报警：%1").arg(latestText)
                       : QStringLiteral("当前有 %1 条活动报警，最新：%2")
                             .arg(summary.activeCount)
                             .arg(latestText);

    clearLastError();
    return summary;
}

bool AlarmDao::updateThresholdRules(const AlarmThreshold &threshold)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before threshold rule update:" << m_lastErrorText;
        return false;
    }

    struct RuleUpdate
    {
        QString ruleCode;
        QString ruleName;
        QString metricCode;
        QString comparator;
        double thresholdValue;
        QString unit;
        QString severity;
        QString description;
    };

    const QList<RuleUpdate> rules = {
        {QStringLiteral("temp_low"), QStringLiteral("温度过低"), QStringLiteral("temperature"), QStringLiteral("lt"), threshold.tempMin, QStringLiteral("°C"), QStringLiteral("warning"), QStringLiteral("环境温度低于设定下限时触发")},
        {QStringLiteral("temp_high"), QStringLiteral("温度过高"), QStringLiteral("temperature"), QStringLiteral("gt"), threshold.tempMax, QStringLiteral("°C"), QStringLiteral("warning"), QStringLiteral("环境温度高于设定上限时触发")},
        {QStringLiteral("humidity_low"), QStringLiteral("湿度过低"), QStringLiteral("humidity"), QStringLiteral("lt"), threshold.humidityMin, QStringLiteral("%"), QStringLiteral("warning"), QStringLiteral("环境湿度低于设定下限时触发")},
        {QStringLiteral("humidity_high"), QStringLiteral("湿度过高"), QStringLiteral("humidity"), QStringLiteral("gt"), threshold.humidityMax, QStringLiteral("%"), QStringLiteral("warning"), QStringLiteral("环境湿度高于设定上限时触发")}};

    static const QString sql =
        "INSERT INTO alarm_rules ("
        "rule_code, rule_name, metric_code, comparator, threshold_value, threshold_unit, "
        "scope_type, severity, cooldown_seconds, is_enabled, description"
        ") VALUES (?, ?, ?, ?, ?, ?, 'global', ?, 300, 1, ?) "
        "ON DUPLICATE KEY UPDATE "
        "rule_name = VALUES(rule_name), "
        "metric_code = VALUES(metric_code), "
        "comparator = VALUES(comparator), "
        "threshold_value = VALUES(threshold_value), "
        "threshold_unit = VALUES(threshold_unit), "
        "severity = VALUES(severity), "
        "is_enabled = VALUES(is_enabled), "
        "description = VALUES(description), "
        "updated_at = CURRENT_TIMESTAMP";

    for (const RuleUpdate &rule : rules)
    {
        const bool ok = databaseManager.exec(
            sql,
            {rule.ruleCode,
             rule.ruleName,
             rule.metricCode,
             rule.comparator,
             rule.thresholdValue,
             rule.unit,
             rule.severity,
             rule.description});
        if (!ok)
        {
            setLastError(databaseManager.lastErrorText());
            qWarning().noquote() << LOG_PREFIX << "Threshold rule update failed:" << m_lastErrorText
                                 << "| rule_code:" << rule.ruleCode;
            return false;
        }
    }

    clearLastError();
    return true;
}

std::optional<AlarmLogEntry> AlarmDao::createEnvironmentAlarm(const QString &alarmCode,
                                                              const QString &alarmType,
                                                              const QString &alarmContent,
                                                              const QString &severity,
                                                              const QString &sourceLocation,
                                                              const QString &triggerMetric,
                                                              double triggerValue,
                                                              const QString &triggerDisplayText,
                                                              const QString &triggerUnit,
                                                              qint64 sourceDeviceId,
                                                              bool *created)
{
    if (created)
    {
        *created = false;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before creating env alarm:" << m_lastErrorText;
        return std::nullopt;
    }

    static const QString checkSql =
        "SELECT id, created_at "
        "FROM alarm_records "
        "WHERE alarm_code = ? AND source_location = ? AND is_active = 1 "
        "ORDER BY created_at DESC LIMIT 1";

    QSqlQuery checkQuery = databaseManager.query(checkSql, {alarmCode, sourceLocation});
    if (!checkQuery.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Existing env alarm query failed:" << m_lastErrorText;
        return std::nullopt;
    }

    const QString finalTriggerText = composeTriggerText(triggerValue, triggerDisplayText, triggerUnit);
    const QVariant deviceValue = sourceDeviceId > 0
                                     ? QVariant::fromValue(sourceDeviceId)
                                     : QVariant::fromValue(qint64(0));

    if (checkQuery.next())
    {
        static const QString updateSql =
            "UPDATE alarm_records "
            "SET alarm_type = ?, alarm_content = ?, severity = ?, source_device_id = ?, "
            "trigger_metric = ?, trigger_value_decimal = ?, trigger_display_text = ?, trigger_unit = ?, "
            "handled_status = 'pending', alarm_source = 'qt_client' "
            "WHERE id = ?";

        const bool ok = databaseManager.exec(
            updateSql,
            {alarmType,
             alarmContent,
             severity,
             deviceValue,
             triggerMetric,
             triggerValue,
             triggerDisplayText,
             triggerUnit,
             checkQuery.value("id").toLongLong()});
        if (!ok)
        {
            setLastError(databaseManager.lastErrorText());
            qWarning().noquote() << LOG_PREFIX << "Update active env alarm failed:" << m_lastErrorText;
            return std::nullopt;
        }

        AlarmLogEntry entry;
        entry.recordId = checkQuery.value("id").toLongLong();
        entry.timestamp = checkQuery.value("created_at").toDateTime();
        entry.type = alarmType;
        entry.triggerValue = finalTriggerText;
        entry.detail = alarmContent;
        entry.severity = severity;
        entry.handledStatus = QStringLiteral("pending");
        entry.isActive = true;
        clearLastError();
        return entry;
    }

    static const QString insertSql =
        "INSERT INTO alarm_records ("
        "alarm_code, alarm_type, alarm_content, severity, source_device_id, source_location, "
        "trigger_metric, trigger_value_decimal, trigger_display_text, trigger_unit, "
        "handled_status, is_active, alarm_source, created_at"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, 'pending', 1, 'qt_client', CURRENT_TIMESTAMP)";

    const bool ok = databaseManager.exec(
        insertSql,
        {alarmCode,
         alarmType,
         alarmContent,
         severity,
         deviceValue,
         sourceLocation,
         triggerMetric,
         triggerValue,
         triggerDisplayText,
         triggerUnit});
    if (!ok)
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Insert env alarm failed:" << m_lastErrorText;
        return std::nullopt;
    }

    QSqlQuery idQuery = databaseManager.query(QStringLiteral("SELECT LAST_INSERT_ID() AS new_id"));
    qint64 recordId = 0;
    if (idQuery.isActive() && idQuery.next())
    {
        recordId = idQuery.value("new_id").toLongLong();
    }

    if (created)
    {
        *created = true;
    }

    AlarmLogEntry entry;
    entry.recordId = recordId;
    entry.timestamp = QDateTime::currentDateTime();
    entry.type = alarmType;
    entry.triggerValue = finalTriggerText;
    entry.detail = alarmContent;
    entry.severity = severity;
    entry.handledStatus = QStringLiteral("pending");
    entry.isActive = true;
    clearLastError();
    return entry;
}

bool AlarmDao::clearEnvironmentAlarm(const QString &alarmCode, const QString &sourceLocation)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before clearing env alarm:" << m_lastErrorText;
        return false;
    }

    static const QString sql =
        "UPDATE alarm_records "
        "SET is_active = 0, handled_status = 'auto_cleared', cleared_at = CURRENT_TIMESTAMP "
        "WHERE alarm_code = ? AND source_location = ? AND is_active = 1";

    if (!databaseManager.exec(sql, {alarmCode, sourceLocation}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Clear env alarm failed:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    return true;
}

bool AlarmDao::clearAlarmLogs()
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before clearing alarm logs:" << m_lastErrorText;
        return false;
    }

    static const QString sql = "DELETE FROM alarm_records";
    if (!databaseManager.exec(sql))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Clear alarm logs failed:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    return true;
}

QString AlarmDao::lastErrorText() const
{
    return m_lastErrorText;
}

void AlarmDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void AlarmDao::clearLastError()
{
    m_lastErrorText.clear();
}
