#pragma once

#include "DatabaseConfig.h"

#include <QHash>
#include <QMutex>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
#include <QStringList>
#include <QVariantList>

class DatabaseManager
{
public:
    static DatabaseManager &instance();

    bool open();
    void close();
    bool isOpen() const;
    QString lastErrorText() const;
    bool testConnection();

    bool exec(const QString &sql, const QVariantList &params = {});
    QSqlQuery query(const QString &sql, const QVariantList &params = {});

    bool beginTransaction();
    bool commit();
    bool rollback();

private:
    DatabaseManager() = default;
    quintptr currentThreadKey() const;
    QString connectionNameForCurrentThread() const;
    QSqlDatabase databaseForCurrentThread() const;
    void registerConnectionName(const QString &connectionName);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    bool ensureOpen(const char *operationName);
    bool prepareAndBind(QSqlQuery &sqlQuery, const QString &sql, const QVariantList &params);
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    DatabaseConfig m_config;
    mutable QMutex m_mutex;
    QHash<quintptr, QString> m_lastErrorTexts;
    QStringList m_connectionNames;
};
