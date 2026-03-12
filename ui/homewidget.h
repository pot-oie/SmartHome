#pragma once

#include <QJsonObject>
#include <QResizeEvent>
#include <QShowEvent>
#include <QList>
#include <QString>
#include <QTimer>
#include <QWidget>

#include "database/dao/EnvRecordDao.h"
#include "services/alarmservice.h"
#include "services/environmentservice.h"
#include "services/settingsservice.h"
#include "services/quickcontrolservice.h"

namespace Ui
{
    class HomeWidget;
}

class QPushButton;

class HomeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWidget(QWidget *parent = nullptr);
    ~HomeWidget();
    void applyLanguage(const QString &languageKey);

signals:
    void alarmTriggered(const QJsonObject &alarmData);

public slots:
    void updateEnvironmentData(double temp, double hum);
    void updateEnvironmentUI(const QJsonObject &data);
    void refreshQuickControls();
    void refreshDeviceStatus();

protected:
    void showEvent(QShowEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onQuickControlClicked();
    void on_btnEditQuickControl_clicked();
    void on_btnGoHome_clicked();

private:
    Ui::HomeWidget *ui;
    QPushButton *m_editQuickControlButton;
    AlarmService m_alarmService;
    EnvironmentService m_environmentService;
    SettingsService m_settingsService;
    QuickControlService m_quickControlService;
    EnvRecordDao m_envRecordDao;
    QTimer *m_environmentRefreshTimer;
    QString m_selectedSceneId;
    QString m_languageKey = QStringLiteral("zh_CN");

    void initConnections();
    void refreshStaticTexts();
    void ensureQuickControlEditButton();
    void applyTemperatureColor(double temperature);
    void updateDeviceStatusLabel(const DeviceStatusSummary &summary);
    void refreshEnvironmentSnapshot();
    void loadQuickControls();
};
