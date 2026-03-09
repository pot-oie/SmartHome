#pragma once

#include <QList>
#include <QString>

class AlarmDao
{
public:
    QList<QString> getRecentAlarmTexts(int limit = 5);
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
