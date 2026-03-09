#include "SystemConfigDao.h"

#include "../DatabaseManager.h"

#include <QDebug>
#include <QSqlQuery>

namespace
{
const char *LOG_PREFIX = "[SystemConfigDao]";
}

QString SystemConfigDao::getSystemStatusText()
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before system config query:" << m_lastErrorText;
        return {};
    }

    static const QString sql =
        "SELECT system_status_text FROM system_configs WHERE config_name = ? LIMIT 1";

    QSqlQuery query = databaseManager.query(sql, {"default"});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "System status query failed:" << m_lastErrorText;
        return {};
    }

    if (!query.next())
    {
        clearLastError();
        qInfo().noquote() << LOG_PREFIX << "No system config row found for default.";
        return {};
    }

    clearLastError();
    return query.value("system_status_text").toString().trimmed();
}

QString SystemConfigDao::lastErrorText() const
{
    return m_lastErrorText;
}

void SystemConfigDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void SystemConfigDao::clearLastError()
{
    m_lastErrorText.clear();
}
