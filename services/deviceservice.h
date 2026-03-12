#pragma once

#include "services/servicemodels.h"

#include <QJsonObject>
#include <QVariant>
#include <QVariantMap>
#include <QStringList>

class DeviceService
{
public:
    QStringList categories() const;
    DeviceList loadDevices() const;
    DeviceList filterDevices(const DeviceList &allDevices, int categoryIndex, const QStringList &categories) const;

    bool supportsAdjust(const DeviceDefinition &device) const;
    QString valueText(const DeviceDefinition &device, int value) const;
    QPair<int, int> sliderRange(const DeviceDefinition &device) const;

    bool updateSwitchState(const QString &deviceId,
                           bool turnOn,
                           QString *errorMessage = nullptr,
                           QString *warningMessage = nullptr) const;
    bool updateDeviceValue(const DeviceDefinition &device,
                           int value,
                           QString *errorMessage = nullptr,
                           QString *warningMessage = nullptr) const;
    QVariantMap loadExtraParams(const QString &deviceId) const;
    bool updateExtraParam(const QString &deviceId,
                          const QString &paramCode,
                          const QVariant &paramValue,
                          const QString &paramName = QString(),
                          const QString &paramUnit = QString(),
                          QString *errorMessage = nullptr,
                          QString *warningMessage = nullptr) const;

    QJsonObject buildSwitchCommand(const QString &deviceId, bool turnOn) const;
    QJsonObject buildSetParamCommand(const DeviceDefinition &device, int value) const;
};
