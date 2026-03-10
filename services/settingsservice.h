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
    bool addDevice(const QString &deviceName, int currentCount, SettingsDeviceEntry *createdDevice, QString *errorText = nullptr) const;
    bool deleteDeviceById(const QString &deviceId, QString *errorText = nullptr) const;
    int mockLatencyMs() const;
};
