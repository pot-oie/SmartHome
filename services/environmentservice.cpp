#include "environmentservice.h"

#include "database/dao/EnvRecordDao.h"
#include "services/settingsservice.h"

#include <QtConcurrent>

EnvironmentService::EnvironmentService(QObject *parent)
    : QObject(parent)
{
}

void EnvironmentService::startPolling(int intervalMs)
{
    if (!m_pollTimer)
    {
        m_pollTimer = new QTimer(this);
        m_watcher = new QFutureWatcher<HomeEnvironmentRefreshResult>(this);
        m_pollTimer->setSingleShot(false);
        connect(m_pollTimer, &QTimer::timeout, this, &EnvironmentService::refreshNow);
        connect(m_watcher, &QFutureWatcher<HomeEnvironmentRefreshResult>::finished,
                this, &EnvironmentService::onWatcherFinished);
    }
    m_pollTimer->start(intervalMs);
    refreshNow();
}

void EnvironmentService::stopPolling()
{
    if (m_pollTimer)
    {
        m_pollTimer->stop();
    }
}

void EnvironmentService::refreshNow()
{
    if (!m_watcher)
    {
        m_watcher = new QFutureWatcher<HomeEnvironmentRefreshResult>(this);
        connect(m_watcher, &QFutureWatcher<HomeEnvironmentRefreshResult>::finished,
                this, &EnvironmentService::onWatcherFinished);
    }
    if (m_watcher->isRunning())
    {
        return;
    }
    m_watcher->setFuture(QtConcurrent::run(&EnvironmentService::doLoad));
}

HomeEnvironmentRefreshResult EnvironmentService::doLoad()
{
    HomeEnvironmentRefreshResult result;

    EnvRecordDao envRecordDao;
    const std::optional<EnvRealtimeSnapshot> snapshot = envRecordDao.getLatestRealtimeSnapshot();
    if (!snapshot.has_value())
    {
        result.success = envRecordDao.lastErrorText().trimmed().isEmpty();
        result.errorText = envRecordDao.lastErrorText();
        return result;
    }

    result.snapshot = snapshot;

    SettingsService settingsService;
    result.deviceStatus = settingsService.loadDeviceStatusSummary();

    result.success = true;
    return result;
}

void EnvironmentService::onWatcherFinished()
{
    emit snapshotRefreshed(m_watcher->result());
}

QString EnvironmentService::temperatureColor(double temperature) const
{
    if (temperature > 28.0)
    {
        return "#f44336";
    }
    if (temperature < 22.0)
    {
        return "#2196F3";
    }
    return "#4CAF50";
}
