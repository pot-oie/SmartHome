#pragma once

#include <QJsonObject>

#include "services/environmentservice.h"
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
    Ui::HomeWidget *ui;
    EnvironmentService m_environmentService;
    void initConnections();
    void applyTemperatureColor(double temperature);
};
