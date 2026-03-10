#include "AlarmDao.h"

#include "../databasemanager.h"

#include <QDateTime>
#include <QDebug>
#include <QSqlQuery>

namespace
{
    const char *LOG_PREFIX = "[AlarmDao]";
}

QList<QString> AlarmDao::getRecentAlarmTexts(int limit)
{
    QList<QString> alarmTexts;
    const int safeLimit = qBound(1, limit, 20);

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Failed to open database before recent alarm query:" << m_lastErrorText;
        return alarmTexts;
    }

    static const QString sql =
        "SELECT created_at, alarm_type, alarm_content "
        "FROM alarm_records ORDER BY created_at DESC LIMIT ?";

    QSqlQuery query = databaseManager.query(sql, {safeLimit});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "Recent alarm query failed:" << m_lastErrorText;
        return alarmTexts;
    }

    while (query.next())
    {
        const QDateTime createdAt = query.value("created_at").toDateTime();
        const QString alarmType = query.value("alarm_type").toString().trimmed();
        const QString alarmContent = query.value("alarm_content").toString().trimmed();

        const QString line = QStringLiteral("[%1] %2 | %3")
                                 .arg(createdAt.toString("yyyy-MM-dd HH:mm:ss"))
                                 .arg(alarmType.isEmpty() ? QStringLiteral("Alarm") : alarmType)
                                 .arg(alarmContent.isEmpty() ? QStringLiteral("No details") : alarmContent);
        alarmTexts.append(line);
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "Loaded recent alarms:" << alarmTexts.size();
    return alarmTexts;
}

QString AlarmDao::lastErrorText() const
{
    return m_lastErrorText;
}

void AlarmDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void AlarmDao::clearLastError()
{
    m_lastErrorText.clear();
}
