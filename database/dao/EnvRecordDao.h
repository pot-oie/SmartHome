#pragma once

#include "services/servicemodels.h"

#include <QPair>
#include <QString>
#include <optional>

class EnvRecordDao
{
public:
    std::optional<QPair<double, double>> getLatestTemperatureAndHumidity();
    std::optional<EnvRealtimeSnapshot> getLatestRealtimeSnapshot();
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
