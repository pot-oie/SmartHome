#include "settingsservice.h"

#include "database/dao/DeviceDao.h"

#include <QRandomGenerator>

QStringList SettingsService::themeOptions() const
{
    return {QStringLiteral("\u6d45\u8272\u4e3b\u9898"), QStringLiteral("\u6df1\u8272\u4e3b\u9898"), QStringLiteral("\u81ea\u52a8")};
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
    DeviceDao dao;
    SettingsDeviceList devicesFromDb = dao.listSettingsDevices();
    if (devicesFromDb.isEmpty())
    {
        dao.ensureDefaultDeviceData();
        devicesFromDb = dao.listSettingsDevices();
    }

    if (!devicesFromDb.isEmpty())
    {
        return devicesFromDb;
    }

    return {
        {"light_living", QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), QStringLiteral("\u7167\u660e\u8bbe\u5907"), "192.168.1.101", true},
        {"light_bedroom", QStringLiteral("\u5367\u5ba4\u706f"), QStringLiteral("\u7167\u660e\u8bbe\u5907"), "192.168.1.102", true},
        {"ac_living", QStringLiteral("\u5ba2\u5385\u7a7a\u8c03"), QStringLiteral("\u7a7a\u8c03\u8bbe\u5907"), "192.168.1.103", true},
        {"curtain_living", QStringLiteral("\u5ba2\u5385\u7a97\u5e18"), QStringLiteral("\u7a97\u5e18\u8bbe\u5907"), "192.168.1.104", true},
        {"lock_door", QStringLiteral("\u524d\u95e8\u667a\u80fd\u9501"), QStringLiteral("\u5b89\u9632\u8bbe\u5907"), "192.168.1.105", true},
        {"camera_01", QStringLiteral("\u5ba2\u5385\u6444\u50cf\u5934"), QStringLiteral("\u5b89\u9632\u8bbe\u5907"), "192.168.1.106", false},
        {"tv_living", QStringLiteral("\u5ba2\u5385\u7535\u89c6"), QStringLiteral("\u5f71\u97f3\u8bbe\u5907"), "192.168.1.107", true}};
}

SettingsDeviceEntry SettingsService::createNewDevice(const QString &deviceName, int currentCount) const
{
    SettingsDeviceEntry device;
    device.id = "device_" + QString::number(currentCount + 100);
    device.name = deviceName.trimmed();
    device.type = QStringLiteral("\u65b0\u8bbe\u5907");
    device.ip = "192.168.1." + QString::number(108 + currentCount);
    device.online = true;
    return device;
}

bool SettingsService::addDevice(const QString &deviceName, int currentCount, SettingsDeviceEntry *createdDevice, QString *errorText) const
{
    SettingsDeviceEntry newDevice = createNewDevice(deviceName, currentCount);

    DeviceDao dao;
    if (!dao.insertDevice(newDevice))
    {
        if (errorText)
        {
            *errorText = dao.lastErrorText();
        }
        return false;
    }

    if (createdDevice)
    {
        *createdDevice = newDevice;
    }
    return true;
}

bool SettingsService::deleteDeviceById(const QString &deviceId, QString *errorText) const
{
    DeviceDao dao;
    if (!dao.deleteDeviceById(deviceId))
    {
        if (errorText)
        {
            *errorText = dao.lastErrorText();
        }
        return false;
    }
    return true;
}

int SettingsService::mockLatencyMs() const
{
    return 10 + QRandomGenerator::global()->bounded(40);
}
