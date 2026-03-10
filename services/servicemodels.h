#pragma once

#include <QDateTime>
#include <QString>
#include <QtGlobal>
#include <QVector>

struct OperationLogEntry
{
    QDateTime timestamp;
    QString user;
    QString operation;
    QString device;
    QString detail;
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

struct SettingsDeviceEntry
{
    QString id;
    QString name;
    QString type;
    QString ip;
    bool online;
};

using SettingsDeviceList = QVector<SettingsDeviceEntry>;

enum class LoginCheckResult
{
    Success,
    EmptyCredential,
    InvalidCredential
};
