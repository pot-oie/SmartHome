#include "DeviceDao.h"

#include "../DatabaseManager.h"

#include <QDebug>
#include <QSqlQuery>

namespace
{
const char *LOG_PREFIX = "[DeviceDao]";
}

int DeviceDao::countAllDevices()
{
    return executeCountQuery("SELECT COUNT(*) FROM devices");
}

int DeviceDao::countOnlineDevices()
{
    return executeCountQuery("SELECT COUNT(*) FROM devices WHERE online_status = ?", {"online"});
}

QString DeviceDao::lastErrorText() const
{
    return m_lastErrorText;
}

int DeviceDao::executeCountQuery(const QString &sql, const QVariantList &params)
{
    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before count query:" << m_lastErrorText;
        return 0;
    }

    QSqlQuery query = databaseManager.query(sql, params);
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Count query failed:" << m_lastErrorText << "| SQL:" << sql;
        return 0;
    }

    if (!query.next())
    {
        clearLastError();
        qInfo().noquote() << LOG_PREFIX << "Count query returned no rows. Fallback to 0.";
        return 0;
    }

    clearLastError();
    return query.value(0).toInt();
}

void DeviceDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void DeviceDao::clearLastError()
{
    m_lastErrorText.clear();
}
