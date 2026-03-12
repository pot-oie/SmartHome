#include "alarmservice.h"

#include "database/dao/AlarmDao.h"
#include "database/dao/EnvRecordDao.h"
#include "database/dao/SystemConfigDao.h"
#include "database/databasemanager.h"
#include "services/usercontext.h"

#include <QtConcurrent>
#include <QList>

namespace
{
    QString formatNumber(double value)
    {
        return QString::number(value, 'f', 2);
    }

    QJsonObject toAlarmJson(const AlarmLogEntry &entry)
    {
        QJsonObject object;
        object.insert(QStringLiteral("type"), entry.type);
        object.insert(QStringLiteral("message"), entry.detail);
        object.insert(QStringLiteral("triggerValue"), entry.triggerValue);
        object.insert(QStringLiteral("severity"), entry.severity);
        object.insert(QStringLiteral("timestamp"), entry.timestamp.toString(Qt::ISODate));
        return object;
    }
}

AlarmThreshold AlarmService::defaultThreshold() const
{
    return {18.0, 30.0, 30.0, 70.0};
}

AlarmThreshold AlarmService::loadThresholds(QString *errorText) const
{
    SystemConfigDao dao;
    const AlarmThreshold threshold = dao.getAlarmThresholds();
    if (!dao.lastErrorText().isEmpty())
    {
        if (errorText)
        {
            *errorText = dao.lastErrorText();
        }
        return defaultThreshold();
    }

    if (errorText)
    {
        errorText->clear();
    }
    return threshold;
}

AlarmLogList AlarmService::loadAlarmLogs(int count, QString *errorText) const
{
    AlarmDao dao;
    const AlarmLogList logs = dao.listAlarmLogs(count);
    if (!dao.lastErrorText().isEmpty())
    {
        if (errorText)
        {
            *errorText = dao.lastErrorText();
        }
        return {};
    }

    if (errorText)
    {
        errorText->clear();
    }
    return logs;
}

AlarmStatusSummary AlarmService::loadAlarmStatus(QString *errorText) const
{
    AlarmDao alarmDao;
    const AlarmStatusSummary activeStatus = alarmDao.getActiveAlarmStatusSummary();
    if (!alarmDao.lastErrorText().isEmpty() && errorText)
    {
        *errorText = alarmDao.lastErrorText();
    }

    if (activeStatus.activeCount > 0)
    {
        return activeStatus;
    }

    SystemConfigDao configDao;
    const AlarmStatusSummary configStatus = configDao.getAlarmStatusSummary();
    if (!configDao.lastErrorText().isEmpty())
    {
        if (errorText)
        {
            *errorText = configDao.lastErrorText();
        }
        return {QStringLiteral("normal"), QStringLiteral("系统正常，无报警"), 0};
    }

    if (errorText)
    {
        errorText->clear();
    }
    return configStatus;
}

bool AlarmService::saveThresholds(const AlarmThreshold &threshold, QString *errorText) const
{
    if (threshold.tempMin >= threshold.tempMax)
    {
        if (errorText)
        {
            *errorText = QStringLiteral("温度下限必须小于温度上限。");
        }
        return false;
    }

    if (threshold.humidityMin >= threshold.humidityMax)
    {
        if (errorText)
        {
            *errorText = QStringLiteral("湿度下限必须小于湿度上限。");
        }
        return false;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return false;
    }

    if (!databaseManager.beginTransaction())
    {
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return false;
    }

    const qint64 updatedBy = UserContext::instance().hasCurrentUser()
                                 ? UserContext::instance().currentUser().id
                                 : 0;

    SystemConfigDao configDao;
    if (!configDao.saveAlarmThresholds(threshold, updatedBy))
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = configDao.lastErrorText();
        }
        return false;
    }

    AlarmDao alarmDao;
    if (!alarmDao.updateThresholdRules(threshold))
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = alarmDao.lastErrorText();
        }
        return false;
    }

    if (!databaseManager.commit())
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return false;
    }

    if (errorText)
    {
        errorText->clear();
    }
    return true;
}

bool AlarmService::clearAlarmLogs(QString *errorText) const
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return false;
    }

    if (!databaseManager.beginTransaction())
    {
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return false;
    }

    AlarmDao dao;
    if (!dao.clearAlarmLogs())
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = dao.lastErrorText();
        }
        return false;
    }

    SystemConfigDao configDao;
    const AlarmStatusSummary normalStatus{QStringLiteral("normal"), QStringLiteral("系统正常，无报警"), 0};
    if (!configDao.updateAlarmStatus(normalStatus))
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = configDao.lastErrorText();
        }
        return false;
    }

    if (!databaseManager.commit())
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return false;
    }

    if (errorText)
    {
        errorText->clear();
    }
    return true;
}

QList<QJsonObject> AlarmService::evaluateEnvironmentSnapshot(const EnvRealtimeSnapshot &snapshot, QString *errorText) const
{
    QList<QJsonObject> triggeredAlarms;

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return triggeredAlarms;
    }

    if (!databaseManager.beginTransaction())
    {
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return triggeredAlarms;
    }

    SystemConfigDao configDao;
    AlarmDao alarmDao;

    const AlarmThreshold threshold = configDao.getAlarmThresholds();
    if (!configDao.lastErrorText().isEmpty())
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = configDao.lastErrorText();
        }
        return triggeredAlarms;
    }

    struct RuleEvaluation
    {
        QString alarmCode;
        QString alarmType;
        QString metricCode;
        double metricValue = 0.0;
        double thresholdValue = 0.0;
        QString unit;
        bool breached = false;
        bool isHigh = false;
    };

    const QList<RuleEvaluation> evaluations = {
        {QStringLiteral("temp_low"), QStringLiteral("温度过低"), QStringLiteral("temperature"), snapshot.temperature, threshold.tempMin, QStringLiteral("°C"), snapshot.temperature < threshold.tempMin, false},
        {QStringLiteral("temp_high"), QStringLiteral("温度过高"), QStringLiteral("temperature"), snapshot.temperature, threshold.tempMax, QStringLiteral("°C"), snapshot.temperature > threshold.tempMax, true},
        {QStringLiteral("humidity_low"), QStringLiteral("湿度过低"), QStringLiteral("humidity"), snapshot.humidity, threshold.humidityMin, QStringLiteral("%"), snapshot.humidity < threshold.humidityMin, false},
        {QStringLiteral("humidity_high"), QStringLiteral("湿度过高"), QStringLiteral("humidity"), snapshot.humidity, threshold.humidityMax, QStringLiteral("%"), snapshot.humidity > threshold.humidityMax, true}};

    const QString locationText = snapshot.locationName.trimmed().isEmpty()
                                     ? QStringLiteral("环境")
                                     : snapshot.locationName.trimmed();
    const QString sourceLocation = locationText;

    for (const RuleEvaluation &evaluation : evaluations)
    {
        if (evaluation.breached)
        {
            const QString metricText = evaluation.metricCode == QStringLiteral("temperature")
                                           ? QStringLiteral("温度")
                                           : QStringLiteral("湿度");
            const QString content = evaluation.isHigh
                                        ? QStringLiteral("%1%2达到%3%4，超过设定上限%5%4")
                                              .arg(locationText)
                                              .arg(metricText)
                                              .arg(formatNumber(evaluation.metricValue))
                                              .arg(evaluation.unit)
                                              .arg(formatNumber(evaluation.thresholdValue))
                                        : QStringLiteral("%1%2降至%3%4，低于设定下限%5%4")
                                              .arg(locationText)
                                              .arg(metricText)
                                              .arg(formatNumber(evaluation.metricValue))
                                              .arg(evaluation.unit)
                                              .arg(formatNumber(evaluation.thresholdValue));

            bool created = false;
            const std::optional<AlarmLogEntry> alarmEntry = alarmDao.createEnvironmentAlarm(
                evaluation.alarmCode,
                evaluation.alarmType,
                content,
                QStringLiteral("warning"),
                sourceLocation,
                evaluation.metricCode,
                evaluation.metricValue,
                formatNumber(evaluation.metricValue),
                evaluation.unit,
                snapshot.sourceDeviceId,
                &created);
            if (!alarmEntry.has_value())
            {
                databaseManager.rollback();
                if (errorText)
                {
                    *errorText = alarmDao.lastErrorText();
                }
                return {};
            }

            if (created)
            {
                triggeredAlarms.append(toAlarmJson(alarmEntry.value()));
            }
        }
        else if (!alarmDao.clearEnvironmentAlarm(evaluation.alarmCode, sourceLocation))
        {
            databaseManager.rollback();
            if (errorText)
            {
                *errorText = alarmDao.lastErrorText();
            }
            return {};
        }
    }

    const AlarmStatusSummary activeStatus = alarmDao.getActiveAlarmStatusSummary();
    if (!alarmDao.lastErrorText().isEmpty())
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = alarmDao.lastErrorText();
        }
        return {};
    }

    const AlarmStatusSummary finalStatus = activeStatus.activeCount > 0
                                               ? activeStatus
                                               : AlarmStatusSummary{QStringLiteral("normal"), QStringLiteral("系统正常，无报警"), 0};
    if (!configDao.updateAlarmStatus(finalStatus))
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = configDao.lastErrorText();
        }
        return {};
    }

    if (!databaseManager.commit())
    {
        databaseManager.rollback();
        if (errorText)
        {
            *errorText = databaseManager.lastErrorText();
        }
        return {};
    }

    if (errorText)
    {
        errorText->clear();
    }
    return triggeredAlarms;
}

QList<QJsonObject> AlarmService::evaluateLatestEnvironment(QString *errorText) const
{
    EnvRecordDao envRecordDao;
    const std::optional<EnvRealtimeSnapshot> snapshot = envRecordDao.getLatestRealtimeSnapshot();
    if (!snapshot.has_value())
    {
        if (errorText)
        {
            *errorText = envRecordDao.lastErrorText();
        }
        return {};
    }

    return evaluateEnvironmentSnapshot(snapshot.value(), errorText);
}

AlarmLogEntry AlarmService::fromAlarmData(const QJsonObject &alarmData) const
{
    AlarmLogEntry entry;
    entry.timestamp = QDateTime::currentDateTime();
    entry.type = alarmData.value("type").toString(QStringLiteral("未知报警"));
    entry.detail = alarmData.value("message").toString(QStringLiteral("无详细信息"));
    entry.triggerValue = alarmData.value("triggerValue").toString();
    entry.severity = alarmData.value("severity").toString(QStringLiteral("warning"));
    return entry;
}

// ── 异步轮询（构造/startPolling/stopPolling/refreshNow/onWatcherFinished） ──────

AlarmService::AlarmService(QObject *parent)
    : QObject(parent)
{
}

void AlarmService::startPolling(int intervalMs)
{
    if (!m_pollTimer)
    {
        m_pollTimer = new QTimer(this);
        m_watcher = new QFutureWatcher<AlarmRuntimePair>(this);
        m_pollTimer->setSingleShot(false);
        connect(m_pollTimer, &QTimer::timeout, this, &AlarmService::refreshNow);
        connect(m_watcher, &QFutureWatcher<AlarmRuntimePair>::finished,
                this, &AlarmService::onWatcherFinished);
    }
    m_pollTimer->start(intervalMs);
    refreshNow();
}

void AlarmService::stopPolling()
{
    if (m_pollTimer)
    {
        m_pollTimer->stop();
    }
}

void AlarmService::refreshNow()
{
    if (!m_watcher)
    {
        m_watcher = new QFutureWatcher<AlarmRuntimePair>(this);
        connect(m_watcher, &QFutureWatcher<AlarmRuntimePair>::finished,
                this, &AlarmService::onWatcherFinished);
    }
    if (m_watcher->isRunning())
    {
        return;
    }
    m_watcher->setFuture(QtConcurrent::run([]() -> AlarmService::AlarmRuntimePair
                                           {
        AlarmService temp;
        return {temp.loadAlarmStatus(), temp.loadAlarmLogs(100)}; }));
}

void AlarmService::onWatcherFinished()
{
    const AlarmRuntimePair pair = m_watcher->result();
    emit runtimeDataRefreshed(pair.first, pair.second);
}
