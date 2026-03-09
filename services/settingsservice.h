#pragma once

#include "services/servicemodels.h"

#include <QStringList>

class SettingsService
{
public:
    QStringList themeOptions() const;
    QString themeKeyByIndex(int index) const;

    SettingsDeviceList loadDefaultDevices() const;
    SettingsDeviceEntry createNewDevice(const QString &deviceName, int currentCount) const;
    int mockLatencyMs() const;
};
