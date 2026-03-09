#pragma once

#include <QString>

class SystemConfigDao
{
public:
    QString getSystemStatusText();
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
