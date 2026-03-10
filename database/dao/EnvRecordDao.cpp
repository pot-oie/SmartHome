#include "EnvRecordDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QSqlQuery>

namespace
{
    const char *LOG_PREFIX = "[EnvRecordDao]";
}

std::optional<QPair<double, double>> EnvRecordDao::getLatestTemperatureAndHumidity()
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before env query:" << m_lastErrorText;
        return std::nullopt;
    }

    static const QString sql =
        "SELECT temperature, humidity FROM env_records ORDER BY created_at DESC LIMIT 1";

    QSqlQuery query = databaseManager.query(sql, {});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Latest env query failed:" << m_lastErrorText;
        return std::nullopt;
    }

    if (!query.next())
    {
        clearLastError();
        qInfo().noquote() << LOG_PREFIX << "No env record found.";
        return std::nullopt;
    }

    clearLastError();
    return QPair<double, double>(query.value("temperature").toDouble(),
                                 query.value("humidity").toDouble());
}

QString EnvRecordDao::lastErrorText() const
{
    return m_lastErrorText;
}

void EnvRecordDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void EnvRecordDao::clearLastError()
{
    m_lastErrorText.clear();
}
