#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

namespace Ui
{
class HomeWidget;
}

class HomeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWidget(QWidget *parent = nullptr);
    ~HomeWidget();

signals:
    void requestQuickControl(QString deviceId, QString cmd);

public slots:
    void updateEnvironmentData(double temp, double hum);
    void updateEnvironmentUI(const QJsonObject &data);

private slots:
    void on_btnGoHome_clicked();
    void onQuickControlClicked();

private:
    void initConnections();
    void loadDashboardData();
    void updateTemperatureLabel(const QString &text);
    void updateHumidityLabel(const QString &text);
    void updateDeviceStatusLabel(int onlineCount, int totalCount);
    void updateSystemStatusText(const QString &systemStatusText);
    void updateRecentAlarmArea(const QList<QString> &alarmTexts);

private:
    Ui::HomeWidget *ui;
};
