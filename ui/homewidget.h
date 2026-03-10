#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QWidget>

#include "services/environmentservice.h"
#include "services/quickcontrolservice.h"

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

public slots:
    void updateEnvironmentData(double temp, double hum);
    void updateEnvironmentUI(const QJsonObject &data);
    void refreshQuickControls();

private slots:
    void onQuickControlClicked();
    void on_btnGoHome_clicked();

private:
    Ui::HomeWidget *ui;
    EnvironmentService m_environmentService;
    QuickControlService m_quickControlService;
    QString m_selectedSceneId;

    void initConnections();
    void applyTemperatureColor(double temperature);

    void loadQuickControls();
};
