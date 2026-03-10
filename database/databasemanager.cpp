#include "databasemanager.h"

#include <QDebug>
#include <QSqlError>
#include <QVariant>

namespace
{
    const char *LOG_PREFIX = "[DatabaseManager]";
}

DatabaseManager &DatabaseManager::instance()
{
    static DatabaseManager manager;
    return manager;
}

DatabaseManager::~DatabaseManager()
{
    close();
}

bool DatabaseManager::open()
{
    if (m_database.isValid() && m_database.isOpen())
    {
        qInfo().noquote() << LOG_PREFIX << "数据库连接已处于打开状态，连接名:" << m_config.connectionName;
        return true;
    }

    if (QSqlDatabase::contains(m_config.connectionName))
    {
        m_database = QSqlDatabase::database(m_config.connectionName);
        qInfo().noquote() << LOG_PREFIX << "复用已有命名连接:" << m_config.connectionName;
    }
    else
    {
        m_database = QSqlDatabase::addDatabase(m_config.driverName, m_config.connectionName);
        qInfo().noquote() << LOG_PREFIX << "创建新的命名连接:" << m_config.connectionName;
    }

    if (!m_database.isValid())
    {
        setLastError("QODBC 驱动无效，请确认 Qt 已正确加载 SQL 驱动。");
        qWarning().noquote() << LOG_PREFIX << "数据库驱动初始化失败。";
        return false;
    }

    m_database.setDatabaseName(m_config.buildConnectionString());

    if (!m_database.open())
    {
        setLastError(m_database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "打开数据库失败:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "数据库连接成功，连接名:" << m_config.connectionName;
    return true;
}

void DatabaseManager::close()
{
    if (!m_database.isValid())
    {
        return;
    }

    if (m_database.isOpen())
    {
        m_database.close();
        qInfo().noquote() << LOG_PREFIX << "数据库连接已关闭，连接名:" << m_config.connectionName;
    }

    clearLastError();
}

bool DatabaseManager::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

QString DatabaseManager::lastErrorText() const
{
    return m_lastErrorText;
}

bool DatabaseManager::testConnection()
{
    if (!open())
    {
        qWarning().noquote() << LOG_PREFIX << "testConnection 执行失败，数据库未连接。";
        return false;
    }

    QSqlQuery sqlQuery = query("SELECT VERSION()", {});
    if (!sqlQuery.isActive())
    {
        qWarning().noquote() << LOG_PREFIX << "testConnection 查询失败:" << m_lastErrorText;
        return false;
    }

    if (sqlQuery.next())
    {
        qInfo().noquote() << LOG_PREFIX << "数据库连通性测试成功，MySQL 版本:" << sqlQuery.value(0).toString();
        return true;
    }

    setLastError("SELECT VERSION() 未返回结果。");
    qWarning().noquote() << LOG_PREFIX << "数据库连通性测试失败:" << m_lastErrorText;
    return false;
}

bool DatabaseManager::exec(const QString &sql, const QVariantList &params)
{
    if (!ensureOpen("exec"))
    {
        return false;
    }

    QSqlQuery sqlQuery(m_database);
    if (!prepareAndBind(sqlQuery, sql, params))
    {
        return false;
    }

    if (!sqlQuery.exec())
    {
        setLastError(sqlQuery.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "执行 SQL 失败:" << m_lastErrorText << "| SQL:" << sql;
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "执行 SQL 成功，影响行数:" << sqlQuery.numRowsAffected() << "| SQL:" << sql;
    return true;
}

QSqlQuery DatabaseManager::query(const QString &sql, const QVariantList &params)
{
    if (!ensureOpen("query"))
    {
        return QSqlQuery();
    }

    QSqlQuery sqlQuery(m_database);
    if (!prepareAndBind(sqlQuery, sql, params))
    {
        return sqlQuery;
    }

    if (!sqlQuery.exec())
    {
        setLastError(sqlQuery.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "查询 SQL 失败:" << m_lastErrorText << "| SQL:" << sql;
        return sqlQuery;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "查询 SQL 成功。| SQL:" << sql;
    return sqlQuery;
}

bool DatabaseManager::beginTransaction()
{
    if (!ensureOpen("beginTransaction"))
    {
        return false;
    }

    if (!m_database.transaction())
    {
        setLastError(m_database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "开启事务失败:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "事务已开启。";
    return true;
}

bool DatabaseManager::commit()
{
    if (!ensureOpen("commit"))
    {
        return false;
    }

    if (!m_database.commit())
    {
        setLastError(m_database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "提交事务失败:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "事务提交成功。";
    return true;
}

bool DatabaseManager::rollback()
{
    if (!ensureOpen("rollback"))
    {
        return false;
    }

    if (!m_database.rollback())
    {
        setLastError(m_database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "回滚事务失败:" << m_lastErrorText;
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "事务回滚成功。";
    return true;
}

bool DatabaseManager::ensureOpen(const char *operationName)
{
    if (isOpen())
    {
        return true;
    }

    setLastError(QString("%1 失败：数据库连接尚未打开。").arg(QString::fromUtf8(operationName)));
    qWarning().noquote() << LOG_PREFIX << m_lastErrorText;
    return false;
}

bool DatabaseManager::prepareAndBind(QSqlQuery &sqlQuery, const QString &sql, const QVariantList &params)
{
    if (!sqlQuery.prepare(sql))
    {
        setLastError(sqlQuery.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "SQL 预处理失败:" << m_lastErrorText << "| SQL:" << sql;
        return false;
    }

    for (const QVariant &param : params)
    {
        sqlQuery.addBindValue(param);
    }

    return true;
}

void DatabaseManager::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void DatabaseManager::clearLastError()
{
    m_lastErrorText.clear();
}
