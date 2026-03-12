#include "settingsservice.h"

#include "database/DatabaseConfig.h"
#include "database/dao/DeviceDao.h"

#include <QElapsedTimer>
#include <QFileInfo>
#include <QIODevice>
#include <QProcess>
#include <QTcpSocket>
#include <QDateTime>
#include <QtConcurrent>

namespace
{
    const QString kSmartHomeTcpHost = QStringLiteral("127.0.0.1");
    const quint16 kSmartHomeTcpPort = 9090;

    QString sanitizeCodeSegment(QString text)
    {
        QString code = text.trimmed().toLower();
        for (int i = 0; i < code.size(); ++i)
        {
            const QChar ch = code.at(i);
            const bool allowed = (ch >= QLatin1Char('a') && ch <= QLatin1Char('z')) || (ch >= QLatin1Char('0') && ch <= QLatin1Char('9')) || ch == QLatin1Char('_');
            if (!allowed)
            {
                code[i] = QLatin1Char('_');
            }
        }

        while (code.contains(QStringLiteral("__")))
        {
            code.replace(QStringLiteral("__"), QStringLiteral("_"));
        }
        return code.trimmed().isEmpty() ? QStringLiteral("device") : code;
    }
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
    const SettingsDeviceList devices = loadDevices();
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

SettingsDeviceList SettingsService::loadDevices() const
{
    DeviceDao dao;
    return dao.listSettingsDevices();
}

SettingsDeviceEntry SettingsService::createNewDevice(const SettingsDeviceEntry &draftDevice, int currentCount) const
{
    SettingsDeviceEntry device = draftDevice;

    device.name = device.name.trimmed();
    device.type = device.type.trimmed().isEmpty() ? QStringLiteral("新设备") : device.type.trimmed();
    device.roomName = device.roomName.trimmed();
    device.protocolType = device.protocolType.trimmed().isEmpty() ? QStringLiteral("simulator") : device.protocolType.trimmed();
    device.manufacturer = device.manufacturer.trimmed().isEmpty() ? QStringLiteral("SmartHome Lab") : device.manufacturer.trimmed();
    device.remarks = device.remarks.trimmed().isEmpty() ? QStringLiteral("新增设备") : device.remarks.trimmed();

    const QString onlineStatus = device.onlineStatus.trimmed().toLower();
    device.onlineStatus = (onlineStatus == QStringLiteral("offline") || onlineStatus == QStringLiteral("error")) ? onlineStatus : QStringLiteral("online");

    const QString switchStatus = device.switchStatus.trimmed().toLower();
    device.switchStatus = (switchStatus == QStringLiteral("on") || switchStatus == QStringLiteral("off"))
                              ? switchStatus
                              : (device.onlineStatus == QStringLiteral("online") ? QStringLiteral("on") : QStringLiteral("off"));

    if (device.id.trimmed().isEmpty())
    {
        const QString typeCode = sanitizeCodeSegment(device.type);
        const qint64 suffix = QDateTime::currentMSecsSinceEpoch() % 1000000;
        device.id = QStringLiteral("%1_%2").arg(typeCode, QString::number(suffix));
    }
    else
    {
        device.id = sanitizeCodeSegment(device.id);
    }

    if (device.ip.trimmed().isEmpty())
    {
        device.ip = QStringLiteral("192.168.1.") + QString::number(108 + currentCount);
    }
    else
    {
        device.ip = device.ip.trimmed();
    }

    if (device.hasSliderConfig && device.sliderMax < device.sliderMin)
    {
        qSwap(device.sliderMin, device.sliderMax);
    }

    return device;
}

bool SettingsService::addDevice(const SettingsDeviceEntry &draftDevice, int currentCount, SettingsDeviceEntry *createdDevice, QString *errorText) const
{
    SettingsDeviceEntry newDevice = createNewDevice(draftDevice, currentCount);

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

bool SettingsService::updateDevice(const QString &originalDeviceId,
                                   const SettingsDeviceEntry &draftDevice,
                                   SettingsDeviceEntry *updatedDevice,
                                   QString *errorText) const
{
    if (originalDeviceId.trimmed().isEmpty())
    {
        if (errorText)
        {
            *errorText = QStringLiteral("待编辑设备编号不能为空。");
        }
        return false;
    }

    SettingsDeviceEntry normalized = draftDevice;
    normalized.id = sanitizeCodeSegment(normalized.id.trimmed().isEmpty() ? originalDeviceId : normalized.id);
    normalized.name = normalized.name.trimmed();
    normalized.type = normalized.type.trimmed().isEmpty() ? QStringLiteral("新设备") : normalized.type.trimmed();
    normalized.ip = normalized.ip.trimmed();
    normalized.roomName = normalized.roomName.trimmed();
    normalized.protocolType = normalized.protocolType.trimmed().isEmpty() ? QStringLiteral("simulator") : normalized.protocolType.trimmed();
    normalized.manufacturer = normalized.manufacturer.trimmed().isEmpty() ? QStringLiteral("SmartHome Lab") : normalized.manufacturer.trimmed();
    normalized.remarks = normalized.remarks.trimmed();
    if (normalized.remarks.isEmpty())
    {
        normalized.remarks = QStringLiteral("设备配置已更新");
    }

    const QString onlineStatus = normalized.onlineStatus.trimmed().toLower();
    normalized.onlineStatus = (onlineStatus == QStringLiteral("offline") || onlineStatus == QStringLiteral("error"))
                                  ? onlineStatus
                                  : QStringLiteral("online");

    const QString switchStatus = normalized.switchStatus.trimmed().toLower();
    normalized.switchStatus = (switchStatus == QStringLiteral("on") || switchStatus == QStringLiteral("off"))
                                  ? switchStatus
                                  : QStringLiteral("on");

    if (normalized.onlineStatus == QStringLiteral("offline"))
    {
        normalized.switchStatus = QStringLiteral("off");
    }

    if (normalized.hasSliderConfig && normalized.sliderMax < normalized.sliderMin)
    {
        qSwap(normalized.sliderMin, normalized.sliderMax);
    }

    DeviceDao dao;
    if (!dao.updateDeviceById(originalDeviceId, normalized))
    {
        if (errorText)
        {
            *errorText = dao.lastErrorText();
        }
        return false;
    }

    if (updatedDevice)
    {
        *updatedDevice = normalized;
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

// ── 异步轮询（构造/startPolling/stopPolling/refreshNow/onWatcherFinished） ──────

SettingsService::SettingsService(QObject *parent)
    : QObject(parent)
{
}

void SettingsService::startPolling(int intervalMs)
{
    if (!m_pollTimer)
    {
        m_pollTimer = new QTimer(this);
        m_watcher = new QFutureWatcher<SettingsDeviceList>(this);
        m_pollTimer->setSingleShot(false);
        connect(m_pollTimer, &QTimer::timeout, this, &SettingsService::refreshNow);
        connect(m_watcher, &QFutureWatcher<SettingsDeviceList>::finished,
                this, &SettingsService::onWatcherFinished);
    }
    m_pollTimer->start(intervalMs);
    refreshNow();
}

void SettingsService::stopPolling()
{
    if (m_pollTimer)
    {
        m_pollTimer->stop();
    }
}

void SettingsService::refreshNow()
{
    if (!m_watcher)
    {
        m_watcher = new QFutureWatcher<SettingsDeviceList>(this);
        connect(m_watcher, &QFutureWatcher<SettingsDeviceList>::finished,
                this, &SettingsService::onWatcherFinished);
    }
    if (m_watcher->isRunning())
    {
        return;
    }
    m_watcher->setFuture(QtConcurrent::run([]()
                                           {
        DeviceDao dao;
        return dao.listSettingsDevices(); }));
}

void SettingsService::onWatcherFinished()
{
    emit devicesRefreshed(m_watcher->result());
}
