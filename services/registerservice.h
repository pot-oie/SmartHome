#pragma once

#include <QString>

class RegisterService
{
public:
    bool registerUser(const QString &username,
                      const QString &password,
                      const QString &displayName);
    QString lastErrorMessage() const;

private:
    QString m_lastErrorMessage;
};
