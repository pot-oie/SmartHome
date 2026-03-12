#pragma once

#include "services/servicemodels.h"

#include <QDateTime>
#include <QFutureWatcher>
#include <QJsonObject>
#include <QObject>
#include <QString>

class HistoryService : public QObject
{
    Q_OBJECT
public:
    explicit HistoryService(QObject *parent = nullptr);

    // 同步方法（保持不变）
    OperationLogList queryOperationLogs(const QDateTime &startTime, const QDateTime &endTime, const QString &deviceType = QString()) const;
    EnvironmentSeries queryEnvironmentSeries(const QDateTime &startTime, const QDateTime &endTime) const;
    bool addOperationLog(const QString &moduleName,
                         const QString &operationType,
                         const QString &operationContent,
                         const QString &result,
                         int resultCode,
                         const QString &deviceId = QString(),
                         const QString &deviceNameSnapshot = QString(),
                         const QJsonObject &requestPayload = QJsonObject(),
                         const QJsonObject &responsePayload = QJsonObject(),
                         QString *errorMessage = nullptr) const;
    bool updateOperationLogResult(qint64 logId,
                                  const QString &result,
                                  int resultCode,
                                  const QJsonObject &responsePayload = QJsonObject(),
                                  QString *errorMessage = nullptr) const;
    bool deleteOperationLog(qint64 logId, QString *errorMessage = nullptr) const;
    bool exportOperationLogsToExcel(const QString &filePath, const OperationLogList &logs, QString *errorMessage = nullptr) const;

    // 异步按需加载（由 UI 主动调用）
    void asyncQueryOperationLogs(const QDateTime &startTime, const QDateTime &endTime, const QString &deviceType = QString());
    void asyncQueryEnvironmentSeries(const QDateTime &startTime, const QDateTime &endTime);

signals:
    void operationLogsReady(OperationLogList logs);
    void environmentSeriesReady(EnvironmentSeries series);

private slots:
    void onLogWatcherFinished();
    void onEnvWatcherFinished();

private:
    QFutureWatcher<OperationLogList> *m_logWatcher = nullptr;
    QFutureWatcher<EnvironmentSeries> *m_envWatcher = nullptr;
};
