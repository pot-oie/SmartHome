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

    // 异步轮询：高频感知环境变化，按需刷新报警状态
    void startPolling(int intervalMs = 300);
    void stopPolling();
    void refreshNow();

signals:
    void alarmsTriggered(QList<QJsonObject> alarms);
    void runtimeDataRefreshed(AlarmStatusSummary status, AlarmLogList logs);

private slots:
    void onWatcherFinished();

private:
    void pollNow();

    struct AlarmRuntimeSnapshot
    {
        QString snapshotSignature;
        bool hasSnapshot = false;
        bool runtimeLoaded = false;
        QList<QJsonObject> triggeredAlarms;
        AlarmStatusSummary status;
        AlarmLogList logs;
        QString errorText;
    };

    QTimer *m_pollTimer = nullptr;
    QFutureWatcher<AlarmRuntimeSnapshot> *m_watcher = nullptr;
    QString m_lastSnapshotSignature;
    qint64 m_lastRuntimeRefreshAtMs = 0;
    bool m_forceRuntimeRefresh = true;
};
