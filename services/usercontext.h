#pragma once

#include "models/User.h"

#include <QString>

class UserContext
{
public:
    static UserContext &instance();

    void setCurrentUser(const User &user,
                        const QString &token,
                        const QString &tokenType = QStringLiteral("Bearer"),
                        int expireSeconds = 0);
    void clear();

    bool hasCurrentUser() const;
    User currentUser() const;
    QString authToken() const;
    QString tokenType() const;
    int expireSeconds() const;
    QString operatorName() const;

private:
    UserContext() = default;

private:
    User m_currentUser;
    QString m_authToken;
    QString m_tokenType = QStringLiteral("Bearer");
    int m_expireSeconds = 0;
    bool m_hasCurrentUser = false;
};
