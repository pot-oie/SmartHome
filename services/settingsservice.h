#pragma once

#include "services/servicemodels.h"

#include <QStringList>
#include <QtGlobal>

struct TcpEndpointTestResult
{
    bool reachable = false;
    int latencyMs = -1;
    QString host;
    quint16 port = 0;
    QString errorText;
};

class SettingsService
{
public:
    QStringList themeOptions() const;
    QString themeKeyByIndex(int index) const;
    QStringList languageOptions() const;
    QString languageKeyByIndex(int index) const;

    DeviceStatusSummary loadDeviceStatusSummary() const;
    SettingsDeviceList loadDevices() const;
    SettingsDeviceEntry createNewDevice(const QString &deviceName, int currentCount) const;
    bool addDevice(const QString &deviceName, int currentCount, SettingsDeviceEntry *createdDevice, QString *errorText = nullptr) const;
    bool deleteDeviceById(const QString &deviceId, QString *errorText = nullptr) const;
    bool backupDatabase(const QString &sqlFilePath, QString *errorText = nullptr) const;
    TcpEndpointTestResult testSmartHomeTcpEndpoint(int timeoutMs = 3000) const;
};
