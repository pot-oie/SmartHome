#pragma once

#include "services/servicemodels.h"

#include <QList>
#include <QString>
#include <optional>

class AlarmDao
{
public:
    QList<QString> getRecentAlarmTexts(int limit = 5);
    AlarmLogList listAlarmLogs(int limit = 50);
    AlarmStatusSummary getActiveAlarmStatusSummary();
    bool updateThresholdRules(const AlarmThreshold &threshold);
    std::optional<AlarmLogEntry> createEnvironmentAlarm(const QString &alarmCode,
                                                        const QString &alarmType,
                                                        const QString &alarmContent,
                                                        const QString &severity,
                                                        const QString &sourceLocation,
                                                        const QString &triggerMetric,
                                                        double triggerValue,
                                                        const QString &triggerDisplayText,
                                                        const QString &triggerUnit,
                                                        qint64 sourceDeviceId,
                                                        bool *created = nullptr);
    bool clearEnvironmentAlarm(const QString &alarmCode, const QString &sourceLocation);
    bool clearAlarmLogs();
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
