#pragma once

#include "services/servicemodels.h"

#include <QJsonObject>
#include <QStringList>

class DeviceService
{
public:
    QStringList categories() const;
    DeviceList loadDefaultDevices() const;
    DeviceList filterDevices(const DeviceList &allDevices, int categoryIndex, const QStringList &categories) const;

    bool supportsAdjust(const QString &deviceType) const;
    QString valueText(const DeviceDefinition &device, int value) const;
    QPair<int, int> sliderRange(const QString &deviceType) const;

    QJsonObject buildSwitchCommand(const QString &deviceId, bool turnOn) const;
    QJsonObject buildSetParamCommand(const DeviceDefinition &device, int value) const;
};
