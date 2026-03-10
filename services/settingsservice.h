#pragma once

#include "services/servicemodels.h"

#include <QStringList>

class SettingsService
{
public:
    QStringList themeOptions() const;
    QString themeKeyByIndex(int index) const;
    QStringList languageOptions() const;
    QString languageKeyByIndex(int index) const;

    SettingsDeviceList loadDefaultDevices() const;
    SettingsDeviceEntry createNewDevice(const QString &deviceName, int currentCount) const;
    bool addDevice(const QString &deviceName, int currentCount, SettingsDeviceEntry *createdDevice, QString *errorText = nullptr) const;
    bool deleteDeviceById(const QString &deviceId, QString *errorText = nullptr) const;
    bool backupDatabase(const QString &sqlFilePath, QString *errorText = nullptr) const;
    int mockLatencyMs() const;
};
