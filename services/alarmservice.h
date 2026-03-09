#pragma once

#include "services/servicemodels.h"

#include <QJsonObject>

class AlarmService
{
public:
    AlarmThreshold defaultThreshold() const;
    AlarmLogList loadSampleLogs(int count) const;
    AlarmLogEntry fromAlarmData(const QJsonObject &alarmData) const;
};
