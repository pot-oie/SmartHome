#pragma once

#include <QJsonObject>
#include <QDateTime>
#include <QShowEvent>
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
    void applyLanguage(const QString &languageKey);

public slots:
    void refreshData();
    void triggerAlarm(const QJsonObject &alarmData);

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void on_btnSaveThresholds_clicked();
    void on_btnClearLogs_clicked();
    void onAlarmRuntimeDataRefreshed(AlarmStatusSummary status, AlarmLogList logs);

private:
    Ui::AlarmWidget *ui;
    AlarmService m_alarmService;
    QString m_languageKey = QStringLiteral("zh_CN");

    void playAlarmAlertTone();
    void loadThresholds();
    void loadAlarmStatus();
    void appendAlarmLogRow(int row, const AlarmLogEntry &entry);

private:
    QDateTime m_lastAlarmDialogAt;
};
