#include "loginservice.h"

#include "database/dao/UserDao.h"
#include "services/usercontext.h"

#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QTimer>
#include <QUrl>

namespace
{
const QUrl kLoginUrl(QStringLiteral("http://127.0.0.1:8080/api/auth/login"));
const int kLoginTimeoutMs = 8000;
}

LoginCheckResult LoginService::checkCredential(const QString &username, const QString &password)
{
    const QString trimmedUsername = username.trimmed();
    const QString trimmedPassword = password.trimmed();
    m_lastErrorMessage.clear();
    UserContext::instance().clear();

    if (trimmedUsername.isEmpty() || trimmedPassword.isEmpty())
    {
        return LoginCheckResult::EmptyCredential;
    }

    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request(kLoginUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject requestBody;
    requestBody.insert(QStringLiteral("username"), trimmedUsername);
    requestBody.insert(QStringLiteral("password"), trimmedPassword);

    QNetworkReply *reply = networkAccessManager.post(
        request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    QEventLoop eventLoop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &eventLoop, [&]() {
        m_lastErrorMessage = QStringLiteral("登录接口请求超时，请确认后端服务已启动。");
        if (reply->isRunning())
        {
            reply->abort();
        }
        eventLoop.quit();
    });

    timeoutTimer.start(kLoginTimeoutMs);
    eventLoop.exec();
    timeoutTimer.stop();

    const QByteArray responseBytes = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError)
    {
        if (m_lastErrorMessage.isEmpty())
        {
            m_lastErrorMessage = networkErrorText.trimmed().isEmpty()
                                     ? QStringLiteral("登录接口调用失败。")
                                     : networkErrorText.trimmed();
        }
        return LoginCheckResult::ServiceError;
    }

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject())
    {
        m_lastErrorMessage = QStringLiteral("登录接口返回了无法解析的响应。");
        return LoginCheckResult::ServiceError;
    }

    const QJsonObject responseObj = responseDoc.object();
    const int code = responseObj.value(QStringLiteral("code")).toInt();
    const QString message = responseObj.value(QStringLiteral("message")).toString().trimmed();
    if (code != 200)
    {
        m_lastErrorMessage = message.isEmpty() ? QStringLiteral("用户名或密码错误。") : message;
        return code == 401 ? LoginCheckResult::InvalidCredential : LoginCheckResult::ServiceError;
    }

    const QJsonObject dataObj = responseObj.value(QStringLiteral("data")).toObject();
    const QString token = dataObj.value(QStringLiteral("token")).toString().trimmed();
    const QString tokenType = dataObj.value(QStringLiteral("tokenType")).toString().trimmed();
    const int expireSeconds = dataObj.value(QStringLiteral("expireSeconds")).toInt();
    if (token.isEmpty())
    {
        m_lastErrorMessage = QStringLiteral("登录成功，但后端未返回有效 token。");
        return LoginCheckResult::ServiceError;
    }

    UserDao userDao;
    const std::optional<User> user = userDao.findByUsername(trimmedUsername);
    if (!user.has_value())
    {
        m_lastErrorMessage = userDao.lastErrorText().trimmed().isEmpty()
                                 ? QStringLiteral("登录成功，但数据库中未找到当前用户信息。")
                                 : userDao.lastErrorText().trimmed();
        return LoginCheckResult::ServiceError;
    }

    UserContext::instance().setCurrentUser(user.value(), token, tokenType, expireSeconds);
    return LoginCheckResult::Success;
}

QString LoginService::errorMessage(LoginCheckResult result) const
{
    switch (result)
    {
    case LoginCheckResult::EmptyCredential:
        return QStringLiteral("请输入用户名和密码！");
    case LoginCheckResult::InvalidCredential:
        return m_lastErrorMessage.isEmpty() ? QStringLiteral("用户名或密码错误！") : m_lastErrorMessage;
    case LoginCheckResult::ServiceError:
        return m_lastErrorMessage.isEmpty() ? QStringLiteral("登录服务暂时不可用。") : m_lastErrorMessage;
    case LoginCheckResult::Success:
    default:
        return QString();
    }
}
