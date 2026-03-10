#include "registerservice.h"

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
const QUrl kRegisterUrl(QStringLiteral("http://127.0.0.1:8080/api/auth/register"));
const int kRegisterTimeoutMs = 8000;
}

bool RegisterService::registerUser(const QString &username,
                                   const QString &password,
                                   const QString &displayName)
{
    const QString trimmedUsername = username.trimmed();
    const QString trimmedPassword = password.trimmed();
    const QString trimmedDisplayName = displayName.trimmed();
    m_lastErrorMessage.clear();

    if (trimmedUsername.isEmpty() || trimmedPassword.isEmpty() || trimmedDisplayName.isEmpty())
    {
        m_lastErrorMessage = QStringLiteral("用户名、密码和显示名称不能为空。");
        return false;
    }

    QNetworkAccessManager networkAccessManager;
    QNetworkRequest request(kRegisterUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));

    QJsonObject requestBody;
    requestBody.insert(QStringLiteral("username"), trimmedUsername);
    requestBody.insert(QStringLiteral("password"), trimmedPassword);
    requestBody.insert(QStringLiteral("displayName"), trimmedDisplayName);

    QNetworkReply *reply = networkAccessManager.post(
        request,
        QJsonDocument(requestBody).toJson(QJsonDocument::Compact));

    QEventLoop eventLoop;
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);

    QObject::connect(reply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
    QObject::connect(&timeoutTimer, &QTimer::timeout, &eventLoop, [&]() {
        m_lastErrorMessage = QStringLiteral("注册接口请求超时，请确认后端服务已启动。");
        if (reply->isRunning())
        {
            reply->abort();
        }
        eventLoop.quit();
    });

    timeoutTimer.start(kRegisterTimeoutMs);
    eventLoop.exec();
    timeoutTimer.stop();

    const QByteArray responseBytes = reply->readAll();
    const QNetworkReply::NetworkError networkError = reply->error();
    const QString networkErrorText = reply->errorString();
    reply->deleteLater();

    if (networkError != QNetworkReply::NoError)
    {
        m_lastErrorMessage = networkErrorText.trimmed().isEmpty()
                                 ? QStringLiteral("注册接口调用失败。")
                                 : networkErrorText.trimmed();
        return false;
    }

    QJsonParseError parseError;
    const QJsonDocument responseDoc = QJsonDocument::fromJson(responseBytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject())
    {
        m_lastErrorMessage = QStringLiteral("注册接口返回了无法解析的响应。");
        return false;
    }

    const QJsonObject responseObj = responseDoc.object();
    const int code = responseObj.value(QStringLiteral("code")).toInt();
    const QString message = responseObj.value(QStringLiteral("message")).toString().trimmed();
    if (code != 200)
    {
        m_lastErrorMessage = message.isEmpty() ? QStringLiteral("注册失败。") : message;
        return false;
    }

    return true;
}

QString RegisterService::lastErrorMessage() const
{
    return m_lastErrorMessage;
}
