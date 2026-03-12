#pragma once

#include "../../services/servicemodels.h"

#include <QList>
#include <QHash>
#include <QString>
#include <QVariantMap>
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
    bool insertDevice(const SettingsDeviceEntry &device);
    bool updateDeviceById(const QString &originalDeviceId, const SettingsDeviceEntry &device);
    bool updateDeviceState(const QString &deviceId,
                           const QString &onlineStatus,
                           const QString &switchStatus,
                           double currentValue,
                           const QString &valueUnit,
                           const QString &modeText = QString());
    QVariantMap loadDeviceExtraParams(const QString &deviceId);
    QHash<QString, QVariantMap> loadDeviceExtraParamsBatch(const QStringList &deviceIds);
    bool upsertDeviceExtraParam(const QString &deviceId,
                                const QString &paramCode,
                                const QVariant &paramValue,
                                const QString &paramName = QString(),
                                const QString &paramUnit = QString());
    bool deleteDeviceById(const QString &deviceId);
    QString lastErrorText() const;

private:
    int executeCountQuery(const QString &sql, const QVariantList &params = {});
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
