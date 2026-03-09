#include "settingsservice.h"

#include <QRandomGenerator>

QStringList SettingsService::themeOptions() const
{
    return {"浅色主题", "深色主题", "自动"};
}

QString SettingsService::themeKeyByIndex(int index) const
{
    switch (index)
    {
    case 0:
        return "light";
    case 1:
        return "dark";
    case 2:
        return "auto";
    default:
        return "light";
    }
}

SettingsDeviceList SettingsService::loadDefaultDevices() const
{
    return {
        {"light_living", "客厅主灯", "照明设备", "192.168.1.101", true},
        {"light_bedroom", "卧室灯", "照明设备", "192.168.1.102", true},
        {"ac_living", "客厅空调", "空调设备", "192.168.1.103", true},
        {"curtain_living", "客厅窗帘", "窗帘设备", "192.168.1.104", true},
        {"lock_door", "前门智能锁", "安防设备", "192.168.1.105", true},
        {"camera_01", "客厅摄像头", "安防设备", "192.168.1.106", false},
        {"tv_living", "客厅电视", "影音设备", "192.168.1.107", true}};
}

SettingsDeviceEntry SettingsService::createNewDevice(const QString &deviceName, int currentCount) const
{
    SettingsDeviceEntry device;
    device.id = "device_" + QString::number(currentCount + 100);
    device.name = deviceName;
    device.type = "新设备";
    device.ip = "192.168.1." + QString::number(108 + currentCount);
    device.online = true;
    return device;
}

int SettingsService::mockLatencyMs() const
{
    return 10 + QRandomGenerator::global()->bounded(40);
}
