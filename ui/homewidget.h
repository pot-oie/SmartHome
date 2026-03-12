#pragma once

#include <QHideEvent>
#include <QResizeEvent>
#include <QShowEvent>
#include <QString>
#include <QWidget>

#include "services/environmentservice.h"
#include "services/quickcontrolservice.h"

namespace Ui
{
    class HomeWidget;
}

class QPushButton;
class QTimer;

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
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onQuickControlClicked();
    void on_btnEditQuickControl_clicked();
    void onEnvironmentSnapshotLoaded(HomeEnvironmentRefreshResult result);

private:
    Ui::HomeWidget *ui;
    QPushButton *m_editQuickControlButton = nullptr;
    EnvironmentService m_environmentService;
    QuickControlService m_quickControlService;
    QString m_selectedSceneId;
    QString m_languageKey = QStringLiteral("zh_CN");
    bool m_themeRefreshScheduled = false;
    QTimer *m_quickControlResizeTimer = nullptr;
    int m_lastQuickControlColumns = -1;

    void initConnections();
    void refreshStaticTexts();
    void scheduleThemeRefresh();
    void ensureQuickControlEditButton();
    int quickControlColumnsForWidth(int width) const;
    void applyTemperatureColor(double temperature);
    void updateDeviceStatusLabel(const DeviceStatusSummary &summary);
    void loadQuickControls();
};
