#include "deviceservice.h"

QStringList DeviceService::categories() const
{
    return {"全部设备", "照明设备", "空调/温控", "窗帘/遮阳", "安防设备", "影音设备"};
}

DeviceList DeviceService::loadDefaultDevices() const
{
    return {
        {"light_living", "客厅主灯", "照明设备", ":/icons/light.svg", true, true, 80},
        {"light_bedroom", "卧室灯", "照明设备", ":/icons/light.svg", true, false, 60},
        {"light_kitchen", "厨房灯", "照明设备", ":/icons/light.svg", true, true, 100},

        {"ac_living", "客厅空调", "空调/温控", ":/icons/ac.svg", true, true, 24},
        {"ac_bedroom", "卧室空调", "空调/温控", ":/icons/ac.svg", true, false, 26},

        {"curtain_living", "客厅窗帘", "窗帘/遮阳", ":/icons/curtains.svg", true, true, 50},
        {"curtain_bedroom", "卧室窗帘", "窗帘/遮阳", ":/icons/curtains.svg", false, false, 0},

        {"lock_door", "前门智能锁", "安防设备", ":/icons/lock.svg", true, true, 0},
        {"camera_01", "客厅摄像头", "安防设备", ":/icons/check.svg", true, true, 0},

        {"tv_living", "客厅电视", "影音设备", ":/icons/tv.svg", true, false, 50}};
}

DeviceList DeviceService::filterDevices(const DeviceList &allDevices, int categoryIndex, const QStringList &categories) const
{
    if (categoryIndex <= 0)
    {
        return allDevices;
    }

    if (categoryIndex >= categories.size())
    {
        return allDevices;
    }

    const QString filterType = categories.at(categoryIndex);
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

bool DeviceService::supportsAdjust(const QString &deviceType) const
{
    return deviceType == "照明设备" || deviceType == "空调/温控" || deviceType == "窗帘/遮阳";
}

QString DeviceService::valueText(const DeviceDefinition &device, int value) const
{
    if (device.type == "空调/温控")
    {
        return QString::number(value) + "°C";
    }
    if (device.type == "照明设备")
    {
        return "亮度: " + QString::number(value) + "%";
    }
    if (device.type == "窗帘/遮阳")
    {
        return "开合: " + QString::number(value) + "%";
    }
    return QString::number(value);
}

QPair<int, int> DeviceService::sliderRange(const QString &deviceType) const
{
    if (deviceType == "空调/温控")
    {
        return {16, 30};
    }
    return {0, 100};
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

    if (device.type == "空调/温控")
    {
        dataObj["param_name"] = "temperature";
    }
    else if (device.type == "照明设备")
    {
        dataObj["param_name"] = "brightness";
    }
    else
    {
        dataObj["param_name"] = "open_level";
    }
    dataObj["param_value"] = value;

    controlCmd["data"] = dataObj;
    return controlCmd;
}
