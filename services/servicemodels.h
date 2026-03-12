#pragma once

#include <optional>
#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QString>
#include <QtGlobal>
#include <QVector>

struct OperationLogEntry
{
    qint64 recordId = 0;
    QDateTime timestamp;
    QString user;
    QString operation;
    QString device;
    QString detail;
    QString result;
};

struct EnvironmentPoint
{
    QDateTime timestamp;
    double temperature;
    double humidity;
};

struct EnvironmentSnapshot
{
    double temperature;
    double humidity;
};

struct EnvRealtimeSnapshot
{
    qint64 recordId = 0;
    QString locationCode;
    QString locationName;
    qint64 sourceDeviceId = 0;
    double temperature = 0.0;
    double humidity = 0.0;
    double pm25 = 0.0;
    double co2 = 0.0;
    QString statusLevel;
    QDateTime updatedAt;
};

using OperationLogList = QVector<OperationLogEntry>;
using EnvironmentSeries = QVector<EnvironmentPoint>;

struct SceneDeviceAction
{
    qint64 recordId = 0;
    QString deviceId;
    QString deviceName;
    QString actionText;
    QString paramText;
};

struct SceneDefinition
{
    QString id;
    QString name;
    QString icon;
    QString description;
    QVector<SceneDeviceAction> actions;
};

using SceneList = QVector<SceneDefinition>;

struct SceneActionExecutionResult
{
    QString deviceId;
    QString deviceName;
    bool success = false;
    QString message;
};

struct SceneExecutionResult
{
    QString sceneId;
    QString sceneName;
    int successCount = 0;
    int failureCount = 0;
    QVector<SceneActionExecutionResult> actionResults;

    bool isSuccess() const
    {
        return failureCount == 0 && successCount > 0;
    }

    bool isPartialSuccess() const
    {
        return successCount > 0 && failureCount > 0;
    }
};

struct DeviceDefinition
{
    QString id;
    QString name;
    QString type;
    QString icon;
    bool isOnline;
    bool isOn;
    int value;
    QString valueUnit;
    bool supportsSlider = false;
    int minValue = 0;
    int maxValue = 100;
};

using DeviceList = QVector<DeviceDefinition>;

struct AlarmThreshold
{
    double tempMin;
    double tempMax;
    double humidityMin;
    double humidityMax;
};

struct AlarmStatusSummary
{
    QString level;
    QString text;
    int activeCount = 0;
};

struct AlarmLogEntry
{
    qint64 recordId = 0;
    QDateTime timestamp;
    QString type;
    QString triggerValue;
    QString detail;
    QString severity;
    QString handledStatus;
    bool isActive = false;
};

using AlarmLogList = QVector<AlarmLogEntry>;

struct DeviceStatusSummary
{
    int onlineCount = 0;
    int totalCount = 0;
};

struct SettingsDeviceEntry
{
    QString id;
    QString name;
    QString type;
    QString ip;
    QString onlineStatus;
    QString roomName;
    QString protocolType;
    QString manufacturer;
    QString switchStatus;
    QString valueUnit;
    QString remarks;
    bool hasSliderConfig = false;
    bool supportsSlider = false;
    double sliderMin = 0.0;
    double sliderMax = 100.0;
};

using SettingsDeviceList = QVector<SettingsDeviceEntry>;

struct HomeEnvironmentRefreshResult
{
    bool success = false;
    QString errorText;
    std::optional<EnvRealtimeSnapshot> snapshot;
    QList<QJsonObject> triggeredAlarms;
    DeviceStatusSummary deviceStatus;
};

enum class LoginCheckResult
{
    Success,
    EmptyCredential,
    InvalidCredential,
    ServiceError
};
