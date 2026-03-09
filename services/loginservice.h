#pragma once

#include "services/servicemodels.h"

#include <QString>

class LoginService
{
public:
    LoginCheckResult checkCredential(const QString &username, const QString &password) const;
    QString errorMessage(LoginCheckResult result) const;
};
