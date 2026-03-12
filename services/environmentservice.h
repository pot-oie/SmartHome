#pragma once

#include "services/servicemodels.h"

#include <QFutureWatcher>
#include <QObject>
#include <QString>
#include <QTimer>

class EnvironmentService : public QObject
{
    Q_OBJECT
public:
    explicit EnvironmentService(QObject *parent = nullptr);

    // UI 工具方法（同步）
    QString temperatureColor(double temperature) const;

    // 轮询控制：由页面 showEvent/hideEvent 驱动
    void startPolling(int intervalMs = 3000);
    void stopPolling();
    void refreshNow();

signals:
    void snapshotRefreshed(HomeEnvironmentRefreshResult result);

private slots:
    void onWatcherFinished();

private:
    static HomeEnvironmentRefreshResult doLoad();

    QTimer *m_pollTimer = nullptr;
    QFutureWatcher<HomeEnvironmentRefreshResult> *m_watcher = nullptr;
};
