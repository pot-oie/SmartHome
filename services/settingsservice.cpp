#include "settingsservice.h"

#include "database/DatabaseConfig.h"
#include "database/dao/DeviceDao.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QIODevice>
#include <QProcess>
#include <QTcpSocket>

namespace
{
const QString kSmartHomeTcpHost = QStringLiteral("127.0.0.1");
const quint16 kSmartHomeTcpPort = 9090;
}

QStringList SettingsService::themeOptions() const
{
    return {QStringLiteral("\u6d45\u8272\u4e3b\u9898"),
            QStringLiteral("\u6df1\u8272\u4e3b\u9898"),
            QStringLiteral("\u81ea\u52a8")};
}

QString SettingsService::themeKeyByIndex(int index) const
{
    switch (index)
    {
    case 0:
        return QStringLiteral("light");
    case 1:
        return QStringLiteral("dark");
    case 2:
        return QStringLiteral("auto");
    default:
        return QStringLiteral("light");
    }
}

QStringList SettingsService::languageOptions() const
{
    return {QStringLiteral("\u7b80\u4f53\u4e2d\u6587"), QStringLiteral("English")};
}

QString SettingsService::languageKeyByIndex(int index) const
{
    switch (index)
    {
    case 0:
        return QStringLiteral("zh_CN");
    case 1:
        return QStringLiteral("en_US");
    default:
        return QStringLiteral("zh_CN");
    }
}

DeviceStatusSummary SettingsService::loadDeviceStatusSummary() const
{
    DeviceStatusSummary summary;
    const SettingsDeviceList devices = loadDefaultDevices();
    summary.totalCount = devices.size();
    for (const SettingsDeviceEntry &device : devices)
    {
        if (device.onlineStatus == QStringLiteral("online"))
        {
            ++summary.onlineCount;
        }
    }

    return summary;
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
        {QStringLiteral("light_living"), QStringLiteral("\u5ba2\u5385\u4e3b\u706f"), QStringLiteral("\u7167\u660e\u8bbe\u5907"), QStringLiteral("192.168.1.101"), QStringLiteral("online")},
        {QStringLiteral("light_bedroom"), QStringLiteral("\u5367\u5ba4\u706f"), QStringLiteral("\u7167\u660e\u8bbe\u5907"), QStringLiteral("192.168.1.102"), QStringLiteral("online")},
        {QStringLiteral("ac_living"), QStringLiteral("\u5ba2\u5385\u7a7a\u8c03"), QStringLiteral("\u7a7a\u8c03\u8bbe\u5907"), QStringLiteral("192.168.1.103"), QStringLiteral("online")},
        {QStringLiteral("curtain_living"), QStringLiteral("\u5ba2\u5385\u7a97\u5e18"), QStringLiteral("\u7a97\u5e18\u8bbe\u5907"), QStringLiteral("192.168.1.104"), QStringLiteral("online")},
        {QStringLiteral("lock_door"), QStringLiteral("\u524d\u95e8\u667a\u80fd\u9501"), QStringLiteral("\u5b89\u9632\u8bbe\u5907"), QStringLiteral("192.168.1.105"), QStringLiteral("online")},
        {QStringLiteral("camera_01"), QStringLiteral("\u5ba2\u5385\u6444\u50cf\u5934"), QStringLiteral("\u5b89\u9632\u8bbe\u5907"), QStringLiteral("192.168.1.106"), QStringLiteral("offline")},
        {QStringLiteral("tv_living"), QStringLiteral("\u5ba2\u5385\u7535\u89c6"), QStringLiteral("\u5f71\u97f3\u8bbe\u5907"), QStringLiteral("192.168.1.107"), QStringLiteral("online")}
    };
}

SettingsDeviceEntry SettingsService::createNewDevice(const QString &deviceName, int currentCount) const
{
    SettingsDeviceEntry device;
    device.id = QStringLiteral("device_") + QString::number(currentCount + 100);
    device.name = deviceName.trimmed();
    device.type = QStringLiteral("\u65b0\u8bbe\u5907");
    device.ip = QStringLiteral("192.168.1.") + QString::number(108 + currentCount);
    device.onlineStatus = QStringLiteral("online");
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
            *errorText = QStringLiteral("\u5907\u4efd\u6587\u4ef6\u8def\u5f84\u4e0d\u80fd\u4e3a\u7a7a\u3002");
        }
        return false;
    }

    const QFileInfo targetInfo(sqlFilePath);
    const QString outputDir = targetInfo.absolutePath();
    if (outputDir.isEmpty())
    {
        if (errorText)
        {
            *errorText = QStringLiteral("\u5907\u4efd\u76ee\u5f55\u65e0\u6548\u3002");
        }
        return false;
    }

    const DatabaseConfig config;
    QStringList args;
    args << QStringLiteral("--host=%1").arg(config.host)
         << QStringLiteral("--port=%1").arg(config.port)
         << QStringLiteral("--user=%1").arg(config.userName)
         << QStringLiteral("--password=%1").arg(config.password)
         << QStringLiteral("--default-character-set=utf8mb4")
         << config.databaseName;

    QProcess process;
    process.setProgram(QStringLiteral("mysqldump"));
    process.setArguments(args);
    process.setWorkingDirectory(outputDir);
    process.setStandardOutputFile(sqlFilePath, QIODevice::Truncate);
    process.start();

    if (!process.waitForStarted(5000))
    {
        if (errorText)
        {
            *errorText = QStringLiteral("\u65e0\u6cd5\u542f\u52a8 mysqldump\uff0c\u8bf7\u786e\u8ba4 MySQL \u5ba2\u6237\u7aef\u5de5\u5177\u5df2\u5b89\u88c5\u5e76\u52a0\u5165 PATH\u3002");
        }
        return false;
    }

    if (!process.waitForFinished(120000))
    {
        process.kill();
        if (errorText)
        {
            *errorText = QStringLiteral("\u5907\u4efd\u8d85\u65f6\uff0c\u8bf7\u7a0d\u540e\u91cd\u8bd5\u3002");
        }
        return false;
    }

    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
    {
        if (errorText)
        {
            const QString stdErr = QString::fromUtf8(process.readAllStandardError()).trimmed();
            *errorText = stdErr.isEmpty() ? QStringLiteral("mysqldump \u6267\u884c\u5931\u8d25\u3002") : stdErr;
        }
        return false;
    }

    return true;
}

TcpEndpointTestResult SettingsService::testSmartHomeTcpEndpoint(int timeoutMs) const
{
    TcpEndpointTestResult result;
    result.host = kSmartHomeTcpHost;
    result.port = kSmartHomeTcpPort;

    QTcpSocket socket;
    QElapsedTimer timer;
    timer.start();

    socket.connectToHost(result.host, result.port);
    if (!socket.waitForConnected(timeoutMs))
    {
        result.errorText = socket.errorString();
        socket.abort();
        return result;
    }

    result.reachable = true;
    result.latencyMs = static_cast<int>(timer.elapsed());

    socket.disconnectFromHost();
    if (socket.state() != QAbstractSocket::UnconnectedState)
    {
        socket.waitForDisconnected(1000);
    }

    return result;
}
