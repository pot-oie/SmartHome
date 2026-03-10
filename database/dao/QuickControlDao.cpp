#include "QuickControlDao.h"
#include "database/databasemanager.h"
#include <QDebug>
#include <QSqlError>

QList<QuickControlRow> QuickControlDao::listAll() const
{
    m_lastErrorText.clear();
    QList<QuickControlRow> result;

    DatabaseManager &db = DatabaseManager::instance();
    QString sql = "SELECT id, target_type, target_id, display_order FROM quick_controls ORDER BY display_order ASC";

    QSqlQuery query = db.query(sql, {});
    if (!query.isActive())
    {
        m_lastErrorText = query.lastError().text();
        qWarning() << "[QuickControlDao] listAll Error:" << m_lastErrorText;
        return result;
    }

    while (query.next())
    {
        QuickControlRow row;
        row.id = query.value("id").toLongLong();
        row.targetType = query.value("target_type").toString();
        row.targetId = query.value("target_id").toLongLong();
        row.displayOrder = query.value("display_order").toInt();
        result.push_back(row);
    }

    return result;
}

bool QuickControlDao::addShortcut(const QString &targetType, long long targetId) const
{
    m_lastErrorText.clear();
    DatabaseManager &db = DatabaseManager::instance();

    // 先获取当前最大的 display_order，保证新增的排在最后
    int nextOrder = 1;
    QSqlQuery maxOrderQuery = db.query("SELECT MAX(display_order) FROM quick_controls", {});
    if (maxOrderQuery.isActive() && maxOrderQuery.next())
    {
        nextOrder = maxOrderQuery.value(0).toInt() + 1;
    }

    // 使用 INSERT IGNORE 防止重复插入报错
    QString sql = "INSERT IGNORE INTO quick_controls (target_type, target_id, display_order) VALUES (?, ?, ?)";
    if (!db.exec(sql, {targetType, targetId, nextOrder}))
    {
        m_lastErrorText = db.lastErrorText();
        qWarning() << "[QuickControlDao] addShortcut Error:" << m_lastErrorText;
        return false;
    }
    return true;
}

bool QuickControlDao::removeShortcut(const QString &targetType, long long targetId) const
{
    m_lastErrorText.clear();
    DatabaseManager &db = DatabaseManager::instance();

    QString sql = "DELETE FROM quick_controls WHERE target_type = ? AND target_id = ?";
    if (!db.exec(sql, {targetType, targetId}))
    {
        m_lastErrorText = db.lastErrorText();
        qWarning() << "[QuickControlDao] removeShortcut Error:" << m_lastErrorText;
        return false;
    }
    return true;
}

bool QuickControlDao::clearAll() const
{
    m_lastErrorText.clear();
    DatabaseManager &db = DatabaseManager::instance();

    QString sql = "DELETE FROM quick_controls";
    if (!db.exec(sql, {}))
    {
        m_lastErrorText = db.lastErrorText();
        qWarning() << "[QuickControlDao] clearAll Error:" << m_lastErrorText;
        return false;
    }
    return true;
}

QString QuickControlDao::lastErrorText() const
{
    return m_lastErrorText;
}