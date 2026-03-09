#include "homewidget.h"
#include "ui_homewidget.h"

#include "../database/dao/AlarmDao.h"
#include "../database/dao/DeviceDao.h"
#include "../database/dao/EnvRecordDao.h"
#include "../database/dao/SystemConfigDao.h"

#include <QDebug>
#include <QStringList>

namespace
{
QString noAlarmText()
{
    return QString::fromUtf8("\xE6\x9A\x82\xE6\x97\xA0\xE6\x8A\xA5\xE8\xAD\xA6\xE4\xBF\xA1\xE6\x81\xAF");
}

QString normalSystemStatusText()
{
    return QString::fromUtf8("\xE7\xB3\xBB\xE7\xBB\x9F\xE8\xBF\x90\xE8\xA1\x8C\xE6\xAD\xA3\xE5\xB8\xB8\xEF\xBC\x8C\xE6\x9A\x82\xE6\x97\xA0\xE6\x8A\xA5\xE8\xAD\xA6\xE4\xBF\xA1\xE6\x81\xAF");
}

QString onlinePrefix()
{
    return QString::fromUtf8("\xE5\x9C\xA8\xE7\xBA\xBF\x3A\x20");
}

QString systemStatusPrefix()
{
    return QString::fromUtf8("\xE7\xB3\xBB\xE7\xBB\x9F\xE7\x8A\xB6\xE6\x80\x81\xEF\xBC\x9A");
}
}

HomeWidget::HomeWidget(QWidget *parent) : QWidget(parent),
                                          ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
    initConnections();
    loadDashboardData();
}

HomeWidget::~HomeWidget()
{
    delete ui;
}

void HomeWidget::initConnections()
{
    // Current stage keeps homepage quick buttons read-only.
}

void HomeWidget::loadDashboardData()
{
    const QString fallbackTemperature = QStringLiteral("--");
    const QString fallbackHumidity = QStringLiteral("--");

    EnvRecordDao envRecordDao;
    const auto envData = envRecordDao.getLatestTemperatureAndHumidity();
    if (envData.has_value())
    {
        updateTemperatureLabel(QStringLiteral("%1 °C").arg(QString::number(envData->first, 'f', 1)));
        updateHumidityLabel(QStringLiteral("%1 %").arg(QString::number(envData->second, 'f', 1)));
    }
    else
    {
        qWarning() << "[HomeWidget] Failed to load env data:" << envRecordDao.lastErrorText();
        updateTemperatureLabel(fallbackTemperature);
        updateHumidityLabel(fallbackHumidity);
    }

    DeviceDao deviceDao;
    const int totalDevices = deviceDao.countAllDevices();
    const int onlineDevices = deviceDao.countOnlineDevices();
    if (!deviceDao.lastErrorText().isEmpty())
    {
        qWarning() << "[HomeWidget] Device statistics fallback due to query issue:" << deviceDao.lastErrorText();
    }
    updateDeviceStatusLabel(onlineDevices, totalDevices);

    SystemConfigDao systemConfigDao;
    QString systemStatusText = systemConfigDao.getSystemStatusText().trimmed();
    if (systemStatusText.isEmpty())
    {
        if (!systemConfigDao.lastErrorText().isEmpty())
        {
            qWarning() << "[HomeWidget] Failed to load system status:" << systemConfigDao.lastErrorText();
        }
        systemStatusText = normalSystemStatusText();
    }
    updateSystemStatusText(systemStatusText);

    AlarmDao alarmDao;
    QList<QString> recentAlarmTexts = alarmDao.getRecentAlarmTexts(5);
    if (recentAlarmTexts.isEmpty())
    {
        if (!alarmDao.lastErrorText().isEmpty())
        {
            qWarning() << "[HomeWidget] Failed to load recent alarms:" << alarmDao.lastErrorText();
        }
        recentAlarmTexts.append(noAlarmText());
    }

    updateRecentAlarmArea(recentAlarmTexts);
}

void HomeWidget::updateTemperatureLabel(const QString &text)
{
    ui->label_temperature->setText(text);
}

void HomeWidget::updateHumidityLabel(const QString &text)
{
    ui->label_humidity->setText(text);
}

void HomeWidget::updateDeviceStatusLabel(int onlineCount, int totalCount)
{
    ui->label_deviceCount->setText(onlinePrefix() + QString::number(onlineCount) + "/" + QString::number(totalCount));
}

void HomeWidget::updateSystemStatusText(const QString &systemStatusText)
{
    ui->label_systemStatus->setText(systemStatusPrefix() + (systemStatusText.isEmpty()
                                                                ? normalSystemStatusText()
                                                                : systemStatusText));
}

void HomeWidget::updateRecentAlarmArea(const QList<QString> &alarmTexts)
{
    QStringList lines;

    if (alarmTexts.isEmpty())
    {
        lines << noAlarmText();
    }
    else
    {
        lines.append(alarmTexts);
    }

    ui->textEdit_alarmLog->setPlainText(lines.join('\n'));
}

void HomeWidget::onQuickControlClicked()
{
    qInfo() << "[HomeWidget] Quick control clicked. Current stage keeps buttons read-only.";
}

void HomeWidget::updateEnvironmentData(double temp, double hum)
{
    qInfo() << "[HomeWidget] Received environment data:" << temp << hum;
    updateTemperatureLabel(QStringLiteral("%1 °C").arg(QString::number(temp, 'f', 1)));
    updateHumidityLabel(QStringLiteral("%1 %").arg(QString::number(hum, 'f', 1)));
}

void HomeWidget::updateEnvironmentUI(const QJsonObject &data)
{
    if (data.contains("temperature"))
    {
        updateTemperatureLabel(QStringLiteral("%1 °C").arg(QString::number(data.value("temperature").toDouble(), 'f', 1)));
    }

    if (data.contains("humidity"))
    {
        updateHumidityLabel(QStringLiteral("%1 %").arg(QString::number(data.value("humidity").toDouble(), 'f', 1)));
    }
}

void HomeWidget::on_btnGoHome_clicked()
{
    qInfo() << "[HomeWidget] Home scene shortcut clicked. No action is bound in this stage.";
}
