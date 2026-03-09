#pragma once

#include "../../models/User.h"

#include <QString>
#include <optional>

class UserDao
{
public:
    std::optional<User> findByUsername(const QString &username);
    bool verifyLogin(const QString &username, const QString &password);
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
