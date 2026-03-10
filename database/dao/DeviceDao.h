#pragma once

#include "../../services/servicemodels.h"

#include <QList>
#include <QString>
#include <QVariantList>

class DeviceDao
{
public:
    struct DeviceCategoryRow
    {
        QString code;
        QString name;
        int order = 0;
    };

    int countAllDevices();
    int countOnlineDevices();
    QList<DeviceCategoryRow> listEnabledCategories();
    DeviceList listDeviceDefinitions();
    SettingsDeviceList listSettingsDevices();
    bool ensureDefaultDeviceData();
    bool insertDevice(const SettingsDeviceEntry &device);
    bool updateDeviceState(const QString &deviceId,
                           const QString &onlineStatus,
                           const QString &switchStatus,
                           double currentValue,
                           const QString &valueUnit,
                           const QString &modeText = QString());
    bool deleteDeviceById(const QString &deviceId);
    QString lastErrorText() const;

private:
    int executeCountQuery(const QString &sql, const QVariantList &params = {});
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
