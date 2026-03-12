#include "databasemanager.h"

#include <QDebug>
#include <QSqlError>
#include <QThread>

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
    QStringList connectionNames;
    {
        QMutexLocker locker(&m_mutex);
        connectionNames = m_connectionNames;
        m_connectionNames.clear();
        m_lastErrorTexts.clear();
    }

    for (const QString &connectionName : connectionNames)
    {
        if (!QSqlDatabase::contains(connectionName))
        {
            continue;
        }

        QSqlDatabase database = QSqlDatabase::database(connectionName, false);
        if (database.isValid() && database.isOpen())
        {
            database.close();
        }
        database = QSqlDatabase();
        QSqlDatabase::removeDatabase(connectionName);
    }
}

bool DatabaseManager::open()
{
    const QString connectionName = connectionNameForCurrentThread();
    QSqlDatabase database;

    if (QSqlDatabase::contains(connectionName))
    {
        database = QSqlDatabase::database(connectionName, false);
        if (database.isValid() && database.isOpen())
        {
            clearLastError();
            qInfo().noquote() << LOG_PREFIX << "数据库连接已处于打开状态，连接名:" << connectionName;
            return true;
        }
    }
    else
    {
        database = QSqlDatabase::addDatabase(m_config.driverName, connectionName);
        registerConnectionName(connectionName);
        qInfo().noquote() << LOG_PREFIX << "创建新的命名连接:" << connectionName;
    }

    if (!database.isValid())
    {
        setLastError(QStringLiteral("QODBC 驱动无效，请确认 Qt 已正确加载 SQL 驱动。"));
        qWarning().noquote() << LOG_PREFIX << "数据库驱动初始化失败。";
        return false;
    }

    database.setDatabaseName(m_config.buildConnectionString());
    if (!database.open())
    {
        setLastError(database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "打开数据库失败:" << lastErrorText();
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "数据库连接成功，连接名:" << connectionName;
    return true;
}

void DatabaseManager::close()
{
    const QString connectionName = connectionNameForCurrentThread();
    if (!QSqlDatabase::contains(connectionName))
    {
        return;
    }

    QSqlDatabase database = QSqlDatabase::database(connectionName, false);
    if (database.isValid() && database.isOpen())
    {
        database.close();
        qInfo().noquote() << LOG_PREFIX << "数据库连接已关闭，连接名:" << connectionName;
    }

    database = QSqlDatabase();
    QSqlDatabase::removeDatabase(connectionName);

    QMutexLocker locker(&m_mutex);
    m_connectionNames.removeAll(connectionName);
    m_lastErrorTexts.remove(currentThreadKey());
}

bool DatabaseManager::isOpen() const
{
    const QSqlDatabase database = databaseForCurrentThread();
    return database.isValid() && database.isOpen();
}

QString DatabaseManager::lastErrorText() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastErrorTexts.value(currentThreadKey());
}

bool DatabaseManager::testConnection()
{
    if (!open())
    {
        qWarning().noquote() << LOG_PREFIX << "testConnection 执行失败，数据库未连接。";
        return false;
    }

    QSqlQuery sqlQuery = query(QStringLiteral("SELECT VERSION()"), {});
    if (!sqlQuery.isActive())
    {
        qWarning().noquote() << LOG_PREFIX << "testConnection 查询失败:" << lastErrorText();
        return false;
    }

    if (sqlQuery.next())
    {
        qInfo().noquote() << LOG_PREFIX << "数据库连通性测试成功，MySQL 版本:" << sqlQuery.value(0).toString();
        return true;
    }

    setLastError(QStringLiteral("SELECT VERSION() 未返回结果。"));
    qWarning().noquote() << LOG_PREFIX << "数据库连通性测试失败:" << lastErrorText();
    return false;
}

bool DatabaseManager::exec(const QString &sql, const QVariantList &params)
{
    if (!ensureOpen("exec"))
    {
        return false;
    }

    QSqlQuery sqlQuery(databaseForCurrentThread());
    if (!prepareAndBind(sqlQuery, sql, params))
    {
        return false;
    }

    if (!sqlQuery.exec())
    {
        setLastError(sqlQuery.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "执行 SQL 失败:" << lastErrorText() << "| SQL:" << sql;
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

    QSqlQuery sqlQuery(databaseForCurrentThread());
    if (!prepareAndBind(sqlQuery, sql, params))
    {
        return sqlQuery;
    }

    if (!sqlQuery.exec())
    {
        setLastError(sqlQuery.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "查询 SQL 失败:" << lastErrorText() << "| SQL:" << sql;
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

    QSqlDatabase database = databaseForCurrentThread();
    if (!database.transaction())
    {
        setLastError(database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "开启事务失败:" << lastErrorText();
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

    QSqlDatabase database = databaseForCurrentThread();
    if (!database.commit())
    {
        setLastError(database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "提交事务失败:" << lastErrorText();
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

    QSqlDatabase database = databaseForCurrentThread();
    if (!database.rollback())
    {
        setLastError(database.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "回滚事务失败:" << lastErrorText();
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "事务回滚成功。";
    return true;
}

quintptr DatabaseManager::currentThreadKey() const
{
    return reinterpret_cast<quintptr>(QThread::currentThreadId());
}

QString DatabaseManager::connectionNameForCurrentThread() const
{
    return QStringLiteral("%1_%2")
        .arg(m_config.connectionName)
        .arg(static_cast<qulonglong>(currentThreadKey()));
}

QSqlDatabase DatabaseManager::databaseForCurrentThread() const
{
    const QString connectionName = connectionNameForCurrentThread();
    if (!QSqlDatabase::contains(connectionName))
    {
        return QSqlDatabase();
    }

    return QSqlDatabase::database(connectionName, false);
}

void DatabaseManager::registerConnectionName(const QString &connectionName)
{
    QMutexLocker locker(&m_mutex);
    if (!m_connectionNames.contains(connectionName))
    {
        m_connectionNames.push_back(connectionName);
    }
}

bool DatabaseManager::ensureOpen(const char *operationName)
{
    if (isOpen())
    {
        return true;
    }

    if (open())
    {
        return true;
    }

    if (lastErrorText().trimmed().isEmpty())
    {
        setLastError(QStringLiteral("%1 失败：数据库连接尚未打开。").arg(QString::fromUtf8(operationName)));
    }
    qWarning().noquote() << LOG_PREFIX << lastErrorText();
    return false;
}

bool DatabaseManager::prepareAndBind(QSqlQuery &sqlQuery, const QString &sql, const QVariantList &params)
{
    sqlQuery.setForwardOnly(true);

    if (!sqlQuery.prepare(sql))
    {
        setLastError(sqlQuery.lastError().text());
        qWarning().noquote() << LOG_PREFIX << "SQL 预处理失败:" << lastErrorText() << "| SQL:" << sql;
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
    QMutexLocker locker(&m_mutex);
    m_lastErrorTexts.insert(currentThreadKey(), errorText);
}

void DatabaseManager::clearLastError()
{
    QMutexLocker locker(&m_mutex);
    m_lastErrorTexts.remove(currentThreadKey());
}
