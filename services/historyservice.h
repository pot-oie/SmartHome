#pragma once

#include "services/servicemodels.h"

#include <QDate>
#include <QString>

class HistoryService
{
public:
    OperationLogList queryOperationLogs(const QDate &startDate, const QDate &endDate) const;
    EnvironmentSeries queryEnvironmentSeries(int hours) const;
    bool exportOperationLogsToCsv(const QString &filePath, const OperationLogList &logs, QString *errorMessage = nullptr) const;
};
