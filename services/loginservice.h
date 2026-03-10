#pragma once

#include "services/servicemodels.h"

#include <QString>

class LoginService
{
public:
    LoginCheckResult checkCredential(const QString &username, const QString &password);
    QString errorMessage(LoginCheckResult result) const;

private:
    QString m_lastErrorMessage;
};
