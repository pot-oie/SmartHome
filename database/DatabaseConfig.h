#pragma once

#include <QString>

struct DatabaseConfig
{
    QString driverName = "QODBC";
    QString odbcDriverName = "{MySQL ODBC 9.6 Unicode Driver}";
    QString host = "82.157.36.245";
    int port = 13306;
    QString databaseName = "smarthome";
    QString userName = "SmartHome";
    QString password = "123456";
    int option = 3;
    QString connectionName = "SmartHomeMySqlConnection";

    QString buildConnectionString() const
    {
        return QString("Driver=%1;Server=%2;Port=%3;Database=%4;UID=%5;PWD=%6;OPTION=%7;")
            .arg(odbcDriverName)
            .arg(host)
            .arg(port)
            .arg(databaseName)
            .arg(userName)
            .arg(password)
            .arg(option);
    }
};
