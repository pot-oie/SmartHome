#pragma once

#include <QPair>
#include <QString>
#include <optional>

class EnvRecordDao
{
public:
    std::optional<QPair<double, double>> getLatestTemperatureAndHumidity();
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
