#include "usercontext.h"

UserContext &UserContext::instance()
{
    static UserContext context;
    return context;
}

void UserContext::setCurrentUser(const User &user,
                                 const QString &token,
                                 const QString &tokenType,
                                 int expireSeconds)
{
    m_currentUser = user;
    m_authToken = token.trimmed();
    m_tokenType = tokenType.trimmed().isEmpty() ? QStringLiteral("Bearer") : tokenType.trimmed();
    m_expireSeconds = expireSeconds;
    m_hasCurrentUser = true;
}

void UserContext::clear()
{
    m_currentUser = User();
    m_authToken.clear();
    m_tokenType = QStringLiteral("Bearer");
    m_expireSeconds = 0;
    m_hasCurrentUser = false;
}

bool UserContext::hasCurrentUser() const
{
    return m_hasCurrentUser;
}

User UserContext::currentUser() const
{
    return m_currentUser;
}

QString UserContext::authToken() const
{
    return m_authToken;
}

QString UserContext::tokenType() const
{
    return m_tokenType;
}

int UserContext::expireSeconds() const
{
    return m_expireSeconds;
}

QString UserContext::operatorName() const
{
    if (!m_currentUser.display_name.trimmed().isEmpty())
    {
        return m_currentUser.display_name.trimmed();
    }

    return m_currentUser.username.trimmed();
}
