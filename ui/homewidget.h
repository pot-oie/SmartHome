#pragma once

#include <QFutureWatcher>
#include <QJsonObject>
#include <QResizeEvent>
#include <QList>
#include <QShowEvent>
#include <QString>
#include <QTimer>
#include <QWidget>

#include <optional>

#include "database/dao/EnvRecordDao.h"
#include "services/alarmservice.h"
#include "services/environmentservice.h"
#include "services/quickcontrolservice.h"
#include "services/settingsservice.h"

namespace Ui
{
    class HomeWidget;
}

class QPushButton;

struct HomeEnvironmentRefreshResult
{
    bool success = false;
    QString errorText;
    std::optional<EnvRealtimeSnapshot> snapshot;
    QList<QJsonObject> triggeredAlarms;
};

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
    void onEnvironmentSnapshotLoaded();
    void onDeviceStatusLoaded();

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
    void refreshDeviceStatusAsync();
    void loadQuickControls();

private:
    Ui::HomeWidget *ui;
    QPushButton *m_editQuickControlButton = nullptr;
    EnvironmentService m_environmentService;
    QuickControlService m_quickControlService;
    QTimer *m_environmentRefreshTimer = nullptr;
    QString m_selectedSceneId;
    QFutureWatcher<HomeEnvironmentRefreshResult> *m_environmentWatcher = nullptr;
    QFutureWatcher<DeviceStatusSummary> *m_deviceStatusWatcher = nullptr;
    int m_environmentRequestId = 0;
    int m_deviceStatusRequestId = 0;
};
