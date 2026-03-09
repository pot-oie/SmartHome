#include "loginservice.h"

LoginCheckResult LoginService::checkCredential(const QString &username, const QString &password) const
{
    if (username.trimmed().isEmpty() || password.trimmed().isEmpty())
    {
        return LoginCheckResult::EmptyCredential;
    }

    if (username == "admin" && password == "123456")
    {
        return LoginCheckResult::Success;
    }

    return LoginCheckResult::InvalidCredential;
}

QString LoginService::errorMessage(LoginCheckResult result) const
{
    switch (result)
    {
    case LoginCheckResult::EmptyCredential:
        return "请输入用户名和密码！";
    case LoginCheckResult::InvalidCredential:
        return "用户名或密码错误！\n\n提示：默认账号 admin / 123456";
    case LoginCheckResult::Success:
    default:
        return QString();
    }
}
