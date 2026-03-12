#pragma once

#include "../../services/servicemodels.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>

class HistoryDao
{
public:
    OperationLogList queryOperationLogs(const QDateTime &startTime, const QDateTime &endTime, const QString &deviceType = QString());
    EnvironmentSeries queryEnvironmentSeries(const QDateTime &startTime, const QDateTime &endTime);
    bool insertOperationLog(const QString &moduleName,
                            const QString &operationType,
                            const QString &operationContent,
                            const QString &result,
                            int resultCode,
                            const QString &deviceId = QString(),
                            const QString &deviceNameSnapshot = QString(),
                            const QJsonObject &requestPayload = QJsonObject(),
                            const QJsonObject &responsePayload = QJsonObject());
    bool updateOperationLogResult(qint64 logId,
                                  const QString &result,
                                  int resultCode,
                                  const QJsonObject &responsePayload = QJsonObject());
    bool deleteOperationLog(qint64 logId);
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
