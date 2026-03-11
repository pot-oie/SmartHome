#pragma once

#include "services/servicemodels.h"

#include <QList>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class AlarmService
{
public:
    AlarmThreshold defaultThreshold() const;
    AlarmThreshold loadThresholds(QString *errorText = nullptr) const;
    AlarmLogList loadAlarmLogs(int count = 50, QString *errorText = nullptr) const;
    AlarmStatusSummary loadAlarmStatus(QString *errorText = nullptr) const;
    bool saveThresholds(const AlarmThreshold &threshold, QString *errorText = nullptr) const;
    bool clearAlarmLogs(QString *errorText = nullptr) const;
    QList<QJsonObject> evaluateEnvironmentSnapshot(const EnvRealtimeSnapshot &snapshot, QString *errorText = nullptr) const;
    QList<QJsonObject> evaluateLatestEnvironment(QString *errorText = nullptr) const;
    AlarmLogEntry fromAlarmData(const QJsonObject &alarmData) const;
};
