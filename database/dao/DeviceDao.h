#pragma once

#include <QString>
#include <QVariantList>

class DeviceDao
{
public:
    int countAllDevices();
    int countOnlineDevices();
    QString lastErrorText() const;

private:
    int executeCountQuery(const QString &sql, const QVariantList &params = {});
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
