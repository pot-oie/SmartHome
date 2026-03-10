#include "settingsservice.h"

#include "database/DatabaseConfig.h"
#include "database/dao/DeviceDao.h"

#include <QFileInfo>
#include <QProcess>
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

QStringList SettingsService::languageOptions() const
{
    return {"简体中文", "English"};
}

QString SettingsService::languageKeyByIndex(int index) const
{
    switch (index)
    {
    case 0:
        return "zh_CN";
    case 1:
        return "en_US";
    default:
        return "zh_CN";
    }
}

SettingsDeviceList SettingsService::loadDefaultDevices() const
{
    DeviceDao dao;
    const SettingsDeviceList devicesFromDb = dao.listSettingsDevices();
    if (!devicesFromDb.isEmpty())
    {
        return devicesFromDb;
    }

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

bool SettingsService::backupDatabase(const QString &sqlFilePath, QString *errorText) const
{
    if (sqlFilePath.trimmed().isEmpty())
    {
        if (errorText)
        {
            *errorText = "备份文件路径不能为空。";
        }
        return false;
    }

    const QFileInfo targetInfo(sqlFilePath);
    const QString outputDir = targetInfo.absolutePath();
    if (outputDir.isEmpty())
    {
        if (errorText)
        {
            *errorText = "备份目录无效。";
        }
        return false;
    }

    const DatabaseConfig config;
    QStringList args;
    args << QString("--host=%1").arg(config.host)
         << QString("--port=%1").arg(config.port)
         << QString("--user=%1").arg(config.userName)
         << QString("--password=%1").arg(config.password)
         << "--default-character-set=utf8mb4"
         << config.databaseName;

    QProcess process;
    process.setProgram("mysqldump");
    process.setArguments(args);
    process.setWorkingDirectory(outputDir);
    process.setStandardOutputFile(sqlFilePath, QIODevice::Truncate);
    process.start();

    if (!process.waitForStarted(5000))
    {
        if (errorText)
        {
            *errorText = "无法启动 mysqldump，请确认 MySQL 客户端工具已安装并已加入 PATH。";
        }
        return false;
    }

    if (!process.waitForFinished(120000))
    {
        process.kill();
        if (errorText)
        {
            *errorText = "备份超时，请稍后重试。";
        }
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        if (errorText)
        {
            const QString stdErr = QString::fromUtf8(process.readAllStandardError()).trimmed();
            *errorText = stdErr.isEmpty() ? "mysqldump 执行失败。" : stdErr;
        }
        return false;
    }

    return true;
}

int SettingsService::mockLatencyMs() const
{
    return 10 + QRandomGenerator::global()->bounded(40);
}
