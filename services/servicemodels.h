#pragma once

#include <QDateTime>
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

struct AlarmLogEntry
{
    QDateTime timestamp;
    QString type;
    QString detail;
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
};

using SettingsDeviceList = QVector<SettingsDeviceEntry>;

enum class LoginCheckResult
{
    Success,
    EmptyCredential,
    InvalidCredential,
    ServiceError
};
