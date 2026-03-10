#include "UserDao.h"

#include "../databasemanager.h"

#include <QDebug>
#include <QSqlQuery>
#include <QVariant>

namespace
{
    const char *LOG_PREFIX = "[UserDao]";
}

std::optional<User> UserDao::findByUsername(const QString &username)
{
    QString trimmedUsername = username.trimmed();
    if (trimmedUsername.isEmpty())
    {
        setLastError("用户名不能为空。");
        qWarning().noquote() << LOG_PREFIX << "查询用户失败：" << m_lastErrorText;
        return std::nullopt;
    }

    DatabaseManager &databaseManager = DatabaseManager::instance();
    if (!databaseManager.isOpen() && !databaseManager.open())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "查询用户前打开数据库失败：" << m_lastErrorText;
        return std::nullopt;
    }

    static const QString sql =
        "SELECT id, username, password_hash, display_name, role, status, created_at, updated_at "
        "FROM users WHERE username = ? LIMIT 1";

    QSqlQuery query = databaseManager.query(sql, {trimmedUsername});
    if (!query.isActive())
    {
        setLastError(databaseManager.lastErrorText());
        qWarning().noquote() << LOG_PREFIX << "按用户名查询用户失败：" << m_lastErrorText
                             << "| username:" << trimmedUsername;
        return std::nullopt;
    }

    if (!query.next())
    {
        clearLastError();
        qInfo().noquote() << LOG_PREFIX << "未找到用户记录，username:" << trimmedUsername;
        return std::nullopt;
    }

    User user;
    user.id = query.value("id").toLongLong();
    user.username = query.value("username").toString();
    user.password_hash = query.value("password_hash").toString();
    user.display_name = query.value("display_name").toString();
    user.role = query.value("role").toString();
    user.status = query.value("status").toString();
    user.created_at = query.value("created_at").toDateTime();
    user.updated_at = query.value("updated_at").toDateTime();

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "查询用户成功，username:" << trimmedUsername;
    return user;
}

bool UserDao::verifyLogin(const QString &username, const QString &password)
{
    QString trimmedUsername = username.trimmed();
    if (trimmedUsername.isEmpty() || password.isEmpty())
    {
        setLastError("用户名和密码不能为空。");
        qWarning().noquote() << LOG_PREFIX << "登录校验失败：" << m_lastErrorText;
        return false;
    }

    std::optional<User> user = findByUsername(trimmedUsername);
    if (!user.has_value())
    {
        if (m_lastErrorText.isEmpty())
        {
            setLastError("用户名不存在。");
        }
        qWarning().noquote() << LOG_PREFIX << "登录校验失败：" << m_lastErrorText
                             << "| username:" << trimmedUsername;
        return false;
    }

    if (user->status != "active")
    {
        setLastError("用户状态不可用。");
        qWarning().noquote() << LOG_PREFIX << "登录校验失败：用户未激活或已禁用。| username:" << trimmedUsername
                             << "| status:" << user->status;
        return false;
    }

    if (user->password_hash != password)
    {
        setLastError("密码错误。");
        qWarning().noquote() << LOG_PREFIX << "登录校验失败：密码不匹配。| username:" << trimmedUsername;
        return false;
    }

    clearLastError();
    qInfo().noquote() << LOG_PREFIX << "登录校验成功，username:" << trimmedUsername
                      << "| display_name:" << user->display_name;
    return true;
}

QString UserDao::lastErrorText() const
{
    return m_lastErrorText;
}

void UserDao::setLastError(const QString &errorText)
{
    m_lastErrorText = errorText;
}

void UserDao::clearLastError()
{
    m_lastErrorText.clear();
}
