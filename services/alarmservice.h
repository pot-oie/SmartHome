#pragma once

#include "services/servicemodels.h"

#include <QFutureWatcher>
#include <QList>
#include <QJsonArray>
#include <QJsonObject>
#include <QObject>
#include <QPair>
#include <QString>
#include <QTimer>

class AlarmService : public QObject
{
    Q_OBJECT
public:
    explicit AlarmService(QObject *parent = nullptr);

    AlarmThreshold defaultThreshold() const;
    AlarmThreshold loadThresholds(QString *errorText = nullptr) const;
    AlarmLogList loadAlarmLogs(int count = 50, QString *errorText = nullptr) const;
    AlarmStatusSummary loadAlarmStatus(QString *errorText = nullptr) const;
    bool saveThresholds(const AlarmThreshold &threshold, QString *errorText = nullptr) const;
    bool clearAlarmLogs(QString *errorText = nullptr) const;
    QList<QJsonObject> evaluateEnvironmentSnapshot(const EnvRealtimeSnapshot &snapshot, QString *errorText = nullptr) const;
    QList<QJsonObject> evaluateLatestEnvironment(QString *errorText = nullptr) const;
    AlarmLogEntry fromAlarmData(const QJsonObject &alarmData) const;

    // 异步轮询：由页面 showEvent/hideEvent 驱动
    void startPolling(int intervalMs = 5000);
    void stopPolling();
    void refreshNow();

signals:
    void runtimeDataRefreshed(AlarmStatusSummary status, AlarmLogList logs);

private slots:
    void onWatcherFinished();

private:
    using AlarmRuntimePair = QPair<AlarmStatusSummary, AlarmLogList>;

    QTimer *m_pollTimer = nullptr;
    QFutureWatcher<AlarmRuntimePair> *m_watcher = nullptr;
};
