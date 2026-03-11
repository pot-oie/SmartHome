#pragma once

#include "services/servicemodels.h"

#include <QDateTime>
#include <QJsonObject>
#include <QString>

class HistoryService
{
public:
    OperationLogList queryOperationLogs(const QDateTime &startTime, const QDateTime &endTime, const QString &deviceType = QString()) const;
    EnvironmentSeries queryEnvironmentSeries(int hours) const;
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
};
