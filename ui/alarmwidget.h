#pragma once

#include <QJsonObject>
#include <QShowEvent>
#include <QTimer>
#include <QWidget>

#include "services/alarmservice.h"
namespace Ui
{
    class AlarmWidget;
}

class AlarmWidget : public QWidget
{
    Q_OBJECT

public:
    explicit AlarmWidget(QWidget *parent = nullptr);
    ~AlarmWidget();

public slots:
    void refreshData();
    void triggerAlarm(const QJsonObject &alarmData);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_btnSaveThresholds_clicked();
    void on_btnClearLogs_clicked();

private:
    Ui::AlarmWidget *ui;
    AlarmService m_alarmService;
    QTimer *m_refreshTimer = nullptr;

    void playAlarmAlertTone();
    void loadThresholds();
    void loadAlarmStatus();
    void loadAlarmLogs();
    void refreshRuntimeData();
    void appendAlarmLogRow(int row, const AlarmLogEntry &entry);
};
