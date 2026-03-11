#pragma once

#include "services/servicemodels.h"

#include <QString>

class SystemConfigDao
{
public:
    AlarmThreshold getAlarmThresholds();
    bool saveAlarmThresholds(const AlarmThreshold &threshold, qint64 updatedBy = 0);
    bool updateAlarmStatus(const AlarmStatusSummary &status);
    AlarmStatusSummary getAlarmStatusSummary();
    QString getSystemStatusText();
    QString lastErrorText() const;

private:
    bool ensureDefaultConfigRow();
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
