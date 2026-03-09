#include "environmentservice.h"

#include <QRandomGenerator>

EnvironmentSnapshot EnvironmentService::generateInitialSnapshot() const
{
    return {26.5, 45.0};
}

EnvironmentSnapshot EnvironmentService::generateNextSnapshot() const
{
    const double temperature = 20.0 + QRandomGenerator::global()->bounded(12.0);
    const double humidity = 30.0 + QRandomGenerator::global()->bounded(40.0);
    return {temperature, humidity};
}

QString EnvironmentService::temperatureColor(double temperature) const
{
    if (temperature > 28.0)
    {
        return "#f44336";
    }
    if (temperature < 22.0)
    {
        return "#2196F3";
    }
    return "#4CAF50";
}
