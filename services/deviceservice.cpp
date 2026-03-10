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

    QString paramNameForDevice(const DeviceDefinition &device)
    {
        if (isClimateType(device.type))
        {
            return QString("temperature");
        }
        if (isLightingType(device.type))
        {
            return QString("brightness");
        }
        if (isCurtainType(device.type))
        {
            return QString("open_level");
        }
        if (device.type.contains(QStringLiteral("\u5f71\u97f3")) || device.name.contains(QStringLiteral("\u7535\u89c6")))
        {
            return QString("volume");
        }
        return QString("value");
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

    DeviceList builtInFallbackDevices()
    {
        return {
            {"light_living", QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), QStringLiteral("\u7167\u660e\u8bbe\u5907"), ":/icons/light.svg", true, true, 80, "%", true, 0, 100},
            {"light_bedroom", QStringLiteral("\u5367\u5ba4\u706f"), QStringLiteral("\u7167\u660e\u8bbe\u5907"), ":/icons/light.svg", true, false, 60, "%", true, 0, 100},
            {"light_kitchen", QStringLiteral("\u53a8\u623f\u706f"), QStringLiteral("\u7167\u660e\u8bbe\u5907"), ":/icons/light.svg", true, true, 100, "%", true, 0, 100},
            {"ac_living", QStringLiteral("\u5ba2\u5385\u7a7a\u8c03"), QStringLiteral("\u7a7a\u8c03\u8bbe\u5907"), ":/icons/ac.svg", true, true, 24, "C", true, 16, 30},
            {"ac_bedroom", QStringLiteral("\u5367\u5ba4\u7a7a\u8c03"), QStringLiteral("\u7a7a\u8c03\u8bbe\u5907"), ":/icons/ac.svg", true, false, 26, "C", true, 16, 30},
            {"curtain_living", QStringLiteral("\u5ba2\u5385\u7a97\u5e18"), QStringLiteral("\u7a97\u5e18\u8bbe\u5907"), ":/icons/curtains.svg", true, true, 100, "%", true, 0, 100},
            {"curtain_bedroom", QStringLiteral("\u5367\u5ba4\u7a97\u5e18"), QStringLiteral("\u7a97\u5e18\u8bbe\u5907"), ":/icons/curtains.svg", false, false, 0, "%", true, 0, 100},
            {"lock_door", QStringLiteral("\u524d\u95e8\u667a\u80fd\u9501"), QStringLiteral("\u5b89\u9632\u8bbe\u5907"), ":/icons/lock.svg", true, true, 1, "", false, 0, 1},
            {"camera_01", QStringLiteral("\u5ba2\u5385\u6444\u50cf\u5934"), QStringLiteral("\u5b89\u9632\u8bbe\u5907"), ":/icons/check.svg", true, true, 0, "", false, 0, 0},
            {"tv_living", QStringLiteral("\u5ba2\u5385\u7535\u89c6"), QStringLiteral("\u5f71\u97f3\u8bbe\u5907"), ":/icons/tv.svg", true, false, 50, "%", true, 0, 100}};
    }

    QStringList builtInFallbackCategories()
    {
        return {
            kAllDevices,
            QStringLiteral("\u7167\u660e\u8bbe\u5907"),
            QStringLiteral("\u7a7a\u8c03\u8bbe\u5907"),
            QStringLiteral("\u7a97\u5e18\u8bbe\u5907"),
            QStringLiteral("\u5b89\u9632\u8bbe\u5907"),
            QStringLiteral("\u5f71\u97f3\u8bbe\u5907")};
    }
}

QStringList DeviceService::categories() const
{
    QStringList result = {kAllDevices};

    DeviceDao dao;
    QList<DeviceDao::DeviceCategoryRow> categoriesFromDb = dao.listEnabledCategories();
    if (categoriesFromDb.isEmpty())
    {
        dao.ensureDefaultDeviceData();
        categoriesFromDb = dao.listEnabledCategories();
    }

    for (const DeviceDao::DeviceCategoryRow &row : categoriesFromDb)
    {
        const QString name = row.name.trimmed();
        if (!name.isEmpty() && name != QStringLiteral("新设备") && name != QStringLiteral("其他设备"))
        {
            result.push_back(name);
        }
    }

    if (result.size() == 1)
    {
        return builtInFallbackCategories();
    }

    return result;
}

DeviceList DeviceService::loadDefaultDevices() const
{
    DeviceDao dao;
    DeviceList devicesFromDb = dao.listDeviceDefinitions();
    if (devicesFromDb.isEmpty())
    {
        dao.ensureDefaultDeviceData();
        devicesFromDb = dao.listDeviceDefinitions();
    }

    if (!devicesFromDb.isEmpty())
    {
        return devicesFromDb;
    }

    return builtInFallbackDevices();
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

    const DeviceList devices = loadDefaultDevices();
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
        const QJsonObject requestPayload = buildSwitchCommand(device.id, turnOn);
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
    const QJsonObject requestPayload = buildSetParamCommand(device, value);
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

bool DeviceService::syncDeviceStatus(const QJsonObject &statusData) const
{
    const QString deviceId = statusData.value("device_id").toString().trimmed();
    if (deviceId.isEmpty())
    {
        return false;
    }

    DeviceList devices = loadDefaultDevices();
    for (const DeviceDefinition &device : devices)
    {
        if (device.id != deviceId)
        {
            continue;
        }

        QString onlineStatus = device.isOnline ? QString("online") : QString("offline");
        QString switchStatus = device.isOn ? QString("on") : QString("off");
        int value = device.value;

        const QString status = statusData.value("status").toString().trimmed();
        if (status == "online" || status == "offline" || status == "error")
        {
            onlineStatus = status;
        }

        const QString currentState = statusData.value("current_state").toString().trimmed();
        if (currentState == "on" || currentState == "off")
        {
            switchStatus = currentState;
        }

        if (statusData.contains("current_value"))
        {
            value = statusData.value("current_value").toInt();
        }

        DeviceDao dao;
        return dao.updateDeviceState(
            deviceId,
            onlineStatus,
            switchStatus,
            value,
            device.valueUnit,
            modeTextForDevice(device, switchStatus == "on", value));
    }

    return false;
}

QJsonObject DeviceService::buildSwitchCommand(const QString &deviceId, bool turnOn) const
{
    QJsonObject controlCmd;
    controlCmd["action"] = "control_single_device";

    QJsonObject dataObj;
    dataObj["device_id"] = deviceId;
    dataObj["command"] = turnOn ? "turn_on" : "turn_off";

    controlCmd["data"] = dataObj;
    return controlCmd;
}

QJsonObject DeviceService::buildSetParamCommand(const DeviceDefinition &device, int value) const
{
    QJsonObject controlCmd;
    controlCmd["action"] = "control_single_device";

    QJsonObject dataObj;
    dataObj["device_id"] = device.id;
    dataObj["command"] = "set_param";
    dataObj["param_name"] = paramNameForDevice(device);
    dataObj["param_value"] = value;

    controlCmd["data"] = dataObj;
    return controlCmd;
}
