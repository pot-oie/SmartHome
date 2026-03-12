#include "deviceservice.h"

#include "database/dao/DeviceDao.h"
#include "historyservice.h"

#include <QDebug>

namespace
{
    const QString kAllDevices = QStringLiteral("\u5168\u90e8\u8bbe\u5907");

    bool isClimateType(const QString &deviceType)
    {
        return deviceType.contains(QStringLiteral("\u7a7a\u8c03")) || deviceType.contains(QStringLiteral("\u6e29\u63a7"));
    }

    bool isLightingType(const QString &deviceType)
    {
        return deviceType.contains(QStringLiteral("\u7167\u660e"));
    }

    bool isCurtainType(const QString &deviceType)
    {
        return deviceType.contains(QStringLiteral("\u7a97\u5e18")) || deviceType.contains(QStringLiteral("\u906e\u9633"));
    }

    QString modeTextForDevice(const DeviceDefinition &device, bool turnOn, int value)
    {
        if (!turnOn)
        {
            return QStringLiteral("\u5173\u95ed");
        }
        if (isClimateType(device.type))
        {
            return QStringLiteral("\u8bbe\u5b9a %1C").arg(value);
        }
        if (isLightingType(device.type))
        {
            return QStringLiteral("\u4eae\u5ea6 %1%").arg(value);
        }
        if (isCurtainType(device.type))
        {
            return QStringLiteral("\u5f00\u542f %1%").arg(value);
        }
        return QStringLiteral("\u5f00\u542f");
    }

    QString operationContentForSwitch(const DeviceDefinition &device, bool turnOn)
    {
        return (turnOn ? QStringLiteral("\u5f00\u542f") : QStringLiteral("\u5173\u95ed")) + QStringLiteral("\u8bbe\u5907\uff1a") + device.name;
    }

    QString operationContentForValue(const DeviceDefinition &device, int value)
    {
        return QStringLiteral("\u8c03\u6574") + device.name + QStringLiteral("\u53c2\u6570\u4e3a ") + QString::number(value) + device.valueUnit;
    }
}

QStringList DeviceService::categories() const
{
    QStringList result = {kAllDevices};

    DeviceDao dao;
    const QList<DeviceDao::DeviceCategoryRow> categoriesFromDb = dao.listEnabledCategories();

    for (const DeviceDao::DeviceCategoryRow &row : categoriesFromDb)
    {
        const QString name = row.name.trimmed();
        if (!name.isEmpty() && name != QStringLiteral("新设备") && name != QStringLiteral("其他设备"))
        {
            result.push_back(name);
        }
    }

    return result;
}

DeviceList DeviceService::loadDevices() const
{
    DeviceDao dao;
    return dao.listDeviceDefinitions();
}

DeviceList DeviceService::filterDevices(const DeviceList &allDevices, int categoryIndex, const QStringList &categories) const
{
    if (categoryIndex <= 0 || categoryIndex >= categories.size())
    {
        return allDevices;
    }

    const QString filterType = categories.at(categoryIndex).trimmed();
    DeviceList filtered;
    for (const DeviceDefinition &device : allDevices)
    {
        if (device.type == filterType)
        {
            filtered.push_back(device);
        }
    }
    return filtered;
}

bool DeviceService::supportsAdjust(const DeviceDefinition &device) const
{
    return device.supportsSlider;
}

QString DeviceService::valueText(const DeviceDefinition &device, int value) const
{
    if (isClimateType(device.type))
    {
        return QString::number(value) + QStringLiteral("C");
    }
    if (isLightingType(device.type))
    {
        return QStringLiteral("\u4eae\u5ea6: ") + QString::number(value) + "%";
    }
    if (isCurtainType(device.type))
    {
        return QStringLiteral("\u5f00\u542f: ") + QString::number(value) + "%";
    }
    if (!device.valueUnit.trimmed().isEmpty())
    {
        return QString::number(value) + device.valueUnit;
    }
    return QString::number(value);
}

QPair<int, int> DeviceService::sliderRange(const DeviceDefinition &device) const
{
    return {device.minValue, device.maxValue};
}

bool DeviceService::updateSwitchState(const QString &deviceId,
                                      bool turnOn,
                                      QString *errorMessage,
                                      QString *warningMessage) const
{
    if (errorMessage)
    {
        errorMessage->clear();
    }
    if (warningMessage)
    {
        warningMessage->clear();
    }

    const DeviceList devices = loadDevices();
    for (const DeviceDefinition &device : devices)
    {
        if (device.id != deviceId)
        {
            continue;
        }

        DeviceDao dao;
        if (!dao.updateDeviceState(
                deviceId,
                device.isOnline ? QString("online") : QString("offline"),
                turnOn ? QString("on") : QString("off"),
                device.value,
                device.valueUnit,
                modeTextForDevice(device, turnOn, device.value)))
        {
            if (errorMessage)
            {
                *errorMessage = dao.lastErrorText();
            }
            return false;
        }

        HistoryService historyService;
        const QJsonObject requestPayload = {
            {"device_id", device.id},
            {"operation", turnOn ? "turn_on" : "turn_off"}};
        const QJsonObject responsePayload = {
            {"device_id", device.id},
            {"result", "success"},
            {"current_state", turnOn ? "on" : "off"},
            {"current_value", device.value}};
        QString historyErrorMessage;
        if (!historyService.addOperationLog(
                "device_control",
                turnOn ? "turn_on" : "turn_off",
                operationContentForSwitch(device, turnOn),
                "success",
                200,
                device.id,
                device.name,
                requestPayload,
                responsePayload,
                &historyErrorMessage))
        {
            const QString finalMessage = historyErrorMessage.trimmed().isEmpty()
                                             ? QStringLiteral("\u8bbe\u5907\u72b6\u6001\u5df2\u66f4\u65b0\uff0c\u4f46\u5199\u5165\u64cd\u4f5c\u5386\u53f2\u5931\u8d25\u3002")
                                             : QStringLiteral("\u8bbe\u5907\u72b6\u6001\u5df2\u66f4\u65b0\uff0c\u4f46\u5199\u5165\u64cd\u4f5c\u5386\u53f2\u5931\u8d25\uff1a") + historyErrorMessage;
            qWarning().noquote() << "[DeviceService]" << finalMessage << "| device_id:" << device.id;
            if (warningMessage)
            {
                *warningMessage = finalMessage;
            }
        }
        return true;
    }

    if (errorMessage)
    {
        *errorMessage = QStringLiteral("\u672a\u627e\u5230\u8981\u63a7\u5236\u7684\u8bbe\u5907\u3002");
    }
    return false;
}

bool DeviceService::updateDeviceValue(const DeviceDefinition &device,
                                      int value,
                                      QString *errorMessage,
                                      QString *warningMessage) const
{
    if (errorMessage)
    {
        errorMessage->clear();
    }
    if (warningMessage)
    {
        warningMessage->clear();
    }

    DeviceDao dao;
    if (!dao.updateDeviceState(
            device.id,
            device.isOnline ? QString("online") : QString("offline"),
            value > 0 ? QString("on") : QString("off"),
            value,
            device.valueUnit,
            modeTextForDevice(device, value > 0, value)))
    {
        if (errorMessage)
        {
            *errorMessage = dao.lastErrorText();
        }
        return false;
    }

    HistoryService historyService;
    const QJsonObject requestPayload = {
        {"device_id", device.id},
        {"operation", "set_param"},
        {"param_value", value}};
    const QJsonObject responsePayload = {
        {"device_id", device.id},
        {"result", "success"},
        {"current_state", value > 0 ? "on" : "off"},
        {"current_value", value}};
    QString historyErrorMessage;
    if (!historyService.addOperationLog(
            "device_control",
            "set_param",
            operationContentForValue(device, value),
            "success",
            200,
            device.id,
            device.name,
            requestPayload,
            responsePayload,
            &historyErrorMessage))
    {
        const QString finalMessage = historyErrorMessage.trimmed().isEmpty()
                                         ? QStringLiteral("\u8bbe\u5907\u53c2\u6570\u5df2\u66f4\u65b0\uff0c\u4f46\u5199\u5165\u64cd\u4f5c\u5386\u53f2\u5931\u8d25\u3002")
                                         : QStringLiteral("\u8bbe\u5907\u53c2\u6570\u5df2\u66f4\u65b0\uff0c\u4f46\u5199\u5165\u64cd\u4f5c\u5386\u53f2\u5931\u8d25\uff1a") + historyErrorMessage;
        qWarning().noquote() << "[DeviceService]" << finalMessage << "| device_id:" << device.id;
        if (warningMessage)
        {
            *warningMessage = finalMessage;
        }
    }
    return true;
}

QVariantMap DeviceService::loadExtraParams(const QString &deviceId) const
{
    DeviceDao dao;
    return dao.loadDeviceExtraParams(deviceId);
}

QHash<QString, QVariantMap> DeviceService::loadExtraParamsBatch(const QStringList &deviceIds) const
{
    DeviceDao dao;
    return dao.loadDeviceExtraParamsBatch(deviceIds);
}

bool DeviceService::updateExtraParam(const QString &deviceId,
                                     const QString &paramCode,
                                     const QVariant &paramValue,
                                     const QString &paramName,
                                     const QString &paramUnit,
                                     QString *errorMessage,
                                     QString *warningMessage) const
{
    if (errorMessage)
    {
        errorMessage->clear();
    }
    if (warningMessage)
    {
        warningMessage->clear();
    }

    DeviceDao dao;
    if (!dao.upsertDeviceExtraParam(deviceId, paramCode, paramValue, paramName, paramUnit))
    {
        if (errorMessage)
        {
            *errorMessage = dao.lastErrorText();
        }
        return false;
    }

    const DeviceList devices = loadDevices();
    for (const DeviceDefinition &device : devices)
    {
        if (device.id != deviceId)
        {
            continue;
        }

        HistoryService historyService;
        const QJsonObject requestPayload = {
            {"device_id", device.id},
            {"operation", "set_extra_param"},
            {"param_code", paramCode},
            {"param_value", QJsonValue::fromVariant(paramValue)}};
        const QJsonObject responsePayload = {
            {"device_id", device.id},
            {"result", "success"},
            {"param_code", paramCode},
            {"param_value", QJsonValue::fromVariant(paramValue)}};

        QString historyErrorMessage;
        if (!historyService.addOperationLog(
                "device_control",
                "set_extra_param",
                QStringLiteral("更新%1参数 %2=%3").arg(device.name, paramCode, paramValue.toString()),
                "success",
                200,
                device.id,
                device.name,
                requestPayload,
                responsePayload,
                &historyErrorMessage))
        {
            const QString finalMessage = historyErrorMessage.trimmed().isEmpty()
                                             ? QStringLiteral("扩展参数已更新，但写入操作历史失败。")
                                             : QStringLiteral("扩展参数已更新，但写入操作历史失败：") + historyErrorMessage;
            qWarning().noquote() << "[DeviceService]" << finalMessage << "| device_id:" << device.id;
            if (warningMessage)
            {
                *warningMessage = finalMessage;
            }
        }
        break;
    }

    return true;
}

QJsonObject DeviceService::buildSwitchCommand(const QString &deviceId, bool turnOn) const
{
    return QJsonObject{{QStringLiteral("device_id"), deviceId},
                       {QStringLiteral("operation"), turnOn ? QStringLiteral("turn_on") : QStringLiteral("turn_off")}};
}

QJsonObject DeviceService::buildSetParamCommand(const DeviceDefinition &device, int value) const
{
    return QJsonObject{{QStringLiteral("device_id"), device.id},
                       {QStringLiteral("operation"), QStringLiteral("set_param")},
                       {QStringLiteral("param_value"), value},
                       {QStringLiteral("device_name"), device.name}};
}
