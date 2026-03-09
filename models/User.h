#pragma once

#include <QDateTime>
#include <QString>
#include <QtGlobal>

struct User
{
    qint64 id = 0;
    QString username;
    QString password_hash;
    QString display_name;
    QString role;
    QString status;
    QDateTime created_at;
    QDateTime updated_at;
};
