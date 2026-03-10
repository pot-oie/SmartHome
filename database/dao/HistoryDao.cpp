#include "HistoryDao.h"

#include "../databasemanager.h"
#include "services/usercontext.h"

#include <QDateTime>
#include <QDebug>
#include <QJsonDocument>
#include <QSqlQuery>
#include <QTime>
#include <QUuid>

namespace
{
    const char *LOG_PREFIX = "[HistoryDao]";

    QVariant jsonVariant(const QJsonObject &payload)
    {
        if (payload.isEmpty())
        {
            return QVariant();
        }
        return QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    }

    QString currentOperatorName(qint64 *userId)
    {
        if (userId)
        {
            *userId = 0;
        }

        const UserContext &userContext = UserContext::instance();
        if (userContext.hasCurrentUser())
        {
            if (userId)
            {
                *userId = userContext.currentUser().id;
            }
            return userContext.operatorName();
        }

        return QString();
    }

    bool isBinaryJsonError(const QString &errorText)
    {
        return errorText.contains(QStringLiteral("CHARACTER SET 'binary'"), Qt::CaseInsensitive)
               || errorText.contains(QStringLiteral("Cannot create a JSON value"), Qt::CaseInsensitive);
    }
}

OperationLogList HistoryDao::queryOperationLogs(const QDate &startDate, const QDate &endDate, const QString &deviceType)
{
    OperationLogList logs;
    if (!startDate.isValid() || !endDate.isValid() || startDate > endDate)
    {
        return logs;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before operation log query:" << m_lastErrorText;
        return logs;
    }

    QString sql =
        "SELECT l.id, l.created_at, COALESCE(l.operator_name, u.display_name, u.username, '') AS operator_name, "
        "COALESCE(l.operation_type, '') AS operation_type, "
        "COALESCE(l.device_name_snapshot, d.device_name, '') AS device_name, "
        "COALESCE(l.operation_content, '') AS operation_content, "
        "COALESCE(l.result, '') AS result "
        "FROM operation_logs l "
        "LEFT JOIN users u ON u.id = l.user_id "
        "LEFT JOIN devices d ON d.id = l.device_id "
        "LEFT JOIN device_categories c ON c.id = d.category_id "
        "WHERE l.created_at >= ? AND l.created_at <= ? ";

    QVariantList params = {
        QDateTime(startDate, QTime(0, 0, 0)),
        QDateTime(endDate, QTime(23, 59, 59))};

    const QString trimmedType = deviceType.trimmed();
    if (!trimmedType.isEmpty() && trimmedType != QStringLiteral("\u5168\u90e8"))
    {
        sql += "AND c.category_name = ? ";
        params.push_back(trimmedType);
    }

    sql += "ORDER BY l.created_at DESC, l.id DESC";

    QSqlQuery query = databaseManager.query(sql, params);
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Operation log query failed:" << m_lastErrorText;
        return logs;
    }

    while (query.next())
    {
        OperationLogEntry entry;
        entry.recordId = query.value("id").toLongLong();
        entry.timestamp = query.value("created_at").toDateTime();
        entry.user = query.value("operator_name").toString();
        entry.operation = query.value("operation_type").toString();
        entry.device = query.value("device_name").toString();
        entry.detail = query.value("operation_content").toString();
        entry.result = query.value("result").toString();
        logs.push_back(entry);
    }

    clearLastError();
    return logs;
}

EnvironmentSeries HistoryDao::queryEnvironmentSeries(int hours)
{
    EnvironmentSeries series;
    if (hours <= 0)
    {
        return series;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before env history query:" << m_lastErrorText;
        return series;
    }

    const QDateTime startTime = QDateTime::currentDateTime().addSecs(-hours * 3600);
    static const QString sql =
        "SELECT created_at, temperature, humidity "
        "FROM env_records WHERE created_at >= ? "
        "ORDER BY created_at ASC, id ASC";

    QSqlQuery query = databaseManager.query(sql, {startTime});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Environment series query failed:" << m_lastErrorText;
        return series;
    }

    while (query.next())
    {
        EnvironmentPoint point;
        point.timestamp = query.value("created_at").toDateTime();
        point.temperature = query.value("temperature").toDouble();
        point.humidity = query.value("humidity").toDouble();
        series.push_back(point);
    }

    clearLastError();
    return series;
}

bool HistoryDao::insertOperationLog(const QString &moduleName,
                                    const QString &operationType,
                                    const QString &operationContent,
                                    const QString &result,
                                    int resultCode,
                                    const QString &deviceId,
                                    const QString &deviceNameSnapshot,
                                    const QJsonObject &requestPayload,
                                    const QJsonObject &responsePayload)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before insert operation log:" << m_lastErrorText;
        return false;
    }

    qint64 userId = 0;
    const QString operatorName = currentOperatorName(&userId);

    QVariant devicePk;
    if (!deviceId.trimmed().isEmpty())
    {
        QSqlQuery deviceQuery = databaseManager.query(
            "SELECT id, device_name FROM devices WHERE device_id = ? LIMIT 1", {deviceId.trimmed()});
        if (deviceQuery.isActive() && deviceQuery.next())
        {
            devicePk = deviceQuery.value("id");
        }
    }

    const QString msgId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    static const QString sql =
        "INSERT INTO operation_logs ("
        "msg_id, user_id, device_id, operator_name, device_name_snapshot, module_name, "
        "operation_type, operation_content, result, result_code, request_payload, response_payload"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, "
        "CAST(? AS CHAR CHARACTER SET utf8mb4), "
        "CAST(? AS CHAR CHARACTER SET utf8mb4))";
    static const QString fallbackSql =
        "INSERT INTO operation_logs ("
        "msg_id, user_id, device_id, operator_name, device_name_snapshot, module_name, "
        "operation_type, operation_content, result, result_code, request_payload, response_payload"
        ") VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, NULL, NULL)";

    const QVariantList baseParams = {
        msgId,
        userId > 0 ? QVariant(userId) : QVariant(),
        devicePk,
        operatorName,
        deviceNameSnapshot,
        moduleName,
        operationType,
        operationContent,
        result,
        resultCode};
    QVariantList params = baseParams;
    params.push_back(jsonVariant(requestPayload));
    params.push_back(jsonVariant(responsePayload));

    if (!databaseManager.exec(sql, params))
    {
        const QString firstError = databaseManager.lastErrorText();
        if (!isBinaryJsonError(firstError) || !databaseManager.exec(fallbackSql, baseParams))
        {
            setLastError(databaseManager.lastErrorText().isEmpty() ? firstError : databaseManager.lastErrorText());
            qWarning().noquote() << LOG_PREFIX << "Insert operation log failed:" << m_lastErrorText;
            return false;
        }

        qWarning().noquote() << LOG_PREFIX << "JSON payload insert failed, fallback to NULL payloads:" << firstError;
    }

    clearLastError();
    return true;
}

bool HistoryDao::updateOperationLogResult(qint64 logId,
                                          const QString &result,
                                          int resultCode,
                                          const QJsonObject &responsePayload)
{
    if (logId <= 0)
    {
        setLastError(QStringLiteral("\u65e0\u6548\u7684\u65e5\u5fd7\u8bb0\u5f55 ID\u3002"));
        return false;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before update operation log:" << m_lastErrorText;
        return false;
    }

    static const QString sql =
        "UPDATE operation_logs "
        "SET result = ?, result_code = ?, response_payload = CAST(? AS CHAR CHARACTER SET utf8mb4) "
        "WHERE id = ?";
    static const QString fallbackSql =
        "UPDATE operation_logs SET result = ?, result_code = ?, response_payload = NULL WHERE id = ?";

    if (!databaseManager.exec(sql, {result, resultCode, jsonVariant(responsePayload), logId}))
    {
        const QString firstError = databaseManager.lastErrorText();
        if (!isBinaryJsonError(firstError)
            || !databaseManager.exec(fallbackSql, {result, resultCode, logId}))
        {
            setLastError(databaseManager.lastErrorText().isEmpty() ? firstError : databaseManager.lastErrorText());
            qWarning().noquote() << LOG_PREFIX << "Update operation log failed:" << m_lastErrorText << "| log_id:" << logId;
            return false;
        }

        qWarning().noquote() << LOG_PREFIX << "JSON payload update failed, fallback to NULL payload:" << firstError << "| log_id:" << logId;
    }

    clearLastError();
    return true;
}

bool HistoryDao::deleteOperationLog(qint64 logId)
{
    if (logId <= 0)
    {
        setLastError(QStringLiteral("\u65e0\u6548\u7684\u65e5\u5fd7\u8bb0\u5f55 ID\u3002"));
        return false;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before delete operation log:" << m_lastErrorText;
        return false;
    }

    if (!databaseManager.exec("DELETE FROM operation_logs WHERE id = ?", {logId}))
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Delete operation log failed:" << m_lastErrorText << "| log_id:" << logId;
        return false;
    }

    clearLastError();
    return true;
}

QString HistoryDao::lastErrorText() const
{
    return m_lastErrorText;
}

void HistoryDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void HistoryDao::clearLastError()
{
    m_lastErrorText.clear();
}
