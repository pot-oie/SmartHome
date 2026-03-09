#pragma once

#include "DatabaseConfig.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QString>
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
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager &) = delete;
    DatabaseManager &operator=(const DatabaseManager &) = delete;

    bool ensureOpen(const char *operationName);
    bool prepareAndBind(QSqlQuery &sqlQuery, const QString &sql, const QVariantList &params);
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    DatabaseConfig m_config;
    QSqlDatabase m_database;
    QString m_lastErrorText;
};
