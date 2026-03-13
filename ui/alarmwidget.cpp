#include "alarmwidget.h"
#include "ui_alarmwidget.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QTimer>

#include <atomic>
#include <chrono>

#ifdef Q_OS_WIN
#include <thread>
#include <windows.h>
#endif

#ifdef Q_OS_WIN
namespace
{
    std::atomic_bool g_alarmTonePlaying{false};
}
#endif

namespace
{
    QString localizedAlarmStatusText(const QString &text, bool isEnglish)
    {
        if (!isEnglish)
        {
            return text;
        }

        if (text.trimmed().isEmpty())
        {
            return QStringLiteral("System normal, no alarms");
        }

        static const QHash<QString, QString> map = {
            {QStringLiteral("系统正常，无报警"), QStringLiteral("System normal, no alarms")},
            {QStringLiteral("系统正常，无报警信息"), QStringLiteral("System normal, no alarms")},
            {QStringLiteral("系统正常，无报警记录"), QStringLiteral("System normal, no alarms")},
            {QStringLiteral("系统运行正常，无报警"), QStringLiteral("System running normally, no alarms")}};
        return map.value(text, text);
    }
}

AlarmWidget::AlarmWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::AlarmWidget)
{
    ui->setupUi(this);

    ui->tableWidget_alarmLogs->setColumnCount(4);
    ui->tableWidget_alarmLogs->setHorizontalHeaderLabels(
        {QStringLiteral("时间"),
         QStringLiteral("报警类型"),
         QStringLiteral("触发值"),
         QStringLiteral("详情")});
    ui->tableWidget_alarmLogs->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_alarmLogs->setAlternatingRowColors(true);
    ui->tableWidget_alarmLogs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_alarmLogs->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_alarmLogs->verticalHeader()->setVisible(false);

    loadThresholds();
    applyLanguage(QStringLiteral("zh_CN"));
    connect(&m_alarmService, &AlarmService::alarmsTriggered,
            this, [this](const QList<QJsonObject> &alarms)
            {
                for (const QJsonObject &alarmData : alarms)
                {
                    triggerAlarm(alarmData);
                }
            });
    connect(&m_alarmService, &AlarmService::runtimeDataRefreshed,
            this, &AlarmWidget::onAlarmRuntimeDataRefreshed);
    m_alarmService.startPolling(300);
}

AlarmWidget::~AlarmWidget()
{
    delete ui;
}

void AlarmWidget::applyLanguage(const QString &languageKey)
{
    if (languageKey.trimmed().isEmpty())
    {
        return;
    }

    m_languageKey = languageKey;
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    ui->groupBox_thresholds->setTitle(isEnglish ? QStringLiteral("Alarm Thresholds") : QStringLiteral("报警阈值设置"));
    ui->label_tempMin->setText(isEnglish ? QStringLiteral("Temp Min (°C):") : QStringLiteral("温度下限 (°C):"));
    ui->label_tempMax->setText(isEnglish ? QStringLiteral("Temp Max (°C):") : QStringLiteral("温度上限 (°C):"));
    ui->label_humidityMin->setText(isEnglish ? QStringLiteral("Humidity Min (%):") : QStringLiteral("湿度下限 (%):"));
    ui->label_humidityMax->setText(isEnglish ? QStringLiteral("Humidity Max (%):") : QStringLiteral("湿度上限 (%):"));
    ui->btnSaveThresholds->setText(isEnglish ? QStringLiteral("Save Thresholds") : QStringLiteral("保存报警阈值设置"));

    ui->groupBox_alarmStatus->setTitle(isEnglish ? QStringLiteral("Current Alarm Status") : QStringLiteral("当前报警状态"));
    ui->groupBox_alarmHistory->setTitle(isEnglish ? QStringLiteral("Alarm History") : QStringLiteral("报警历史记录"));
    ui->btnClearLogs->setText(isEnglish ? QStringLiteral("Clear History") : QStringLiteral("清空报警历史"));
    ui->tableWidget_alarmLogs->setHorizontalHeaderLabels(isEnglish
                                                             ? QStringList{QStringLiteral("Time"), QStringLiteral("Type"), QStringLiteral("Trigger"), QStringLiteral("Details")}
                                                             : QStringList{QStringLiteral("时间"), QStringLiteral("报警类型"), QStringLiteral("触发值"), QStringLiteral("详情")});
    if (ui->label_alarmText->text().contains(QStringLiteral("系统正常")) || ui->label_alarmText->text().contains(QStringLiteral("System normal")))
    {
        ui->label_alarmText->setText(isEnglish ? QStringLiteral("System normal, no alarms") : QStringLiteral("系统正常，无报警"));
    }
}

void AlarmWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshData();
}

void AlarmWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
}

void AlarmWidget::playAlarmAlertTone()
{
#ifdef Q_OS_WIN
    if (g_alarmTonePlaying.exchange(true))
    {
        return;
    }

    std::thread([]()
                {
                    for (int i = 0; i < 8; ++i)
                    {
                        Beep((i % 2 == 0) ? 1600 : 1200, 250);
                        std::this_thread::sleep_for(std::chrono::milliseconds(120));
                    }
                    g_alarmTonePlaying = false; })
        .detach();
#else
    for (int i = 0; i < 8; ++i)
    {
        QTimer::singleShot(i * 375, this, []()
                           { QApplication::beep(); });
    }
#endif
}

void AlarmWidget::refreshData()
{
    loadThresholds();
    m_alarmService.refreshNow();
}

void AlarmWidget::loadThresholds()
{
    QString errorText;
    const AlarmThreshold threshold = m_alarmService.loadThresholds(&errorText);
    ui->doubleSpinBox_tempMin->setValue(threshold.tempMin);
    ui->doubleSpinBox_tempMax->setValue(threshold.tempMax);
    ui->doubleSpinBox_humidityMin->setValue(threshold.humidityMin);
    ui->doubleSpinBox_humidityMax->setValue(threshold.humidityMax);

    if (!errorText.isEmpty())
    {
        qWarning() << "加载报警阈值失败，已回退默认值:" << errorText;
    }
}

void AlarmWidget::loadAlarmStatus()
{
    QString errorText;
    const AlarmStatusSummary status = m_alarmService.loadAlarmStatus(&errorText);
    const bool hasActiveAlarm = status.activeCount > 0 || status.level == QStringLiteral("warning") || status.level == QStringLiteral("critical");

    ui->label_alarmIndicator->setText(QStringLiteral("●"));
    ui->label_alarmIndicator->setStyleSheet(
        hasActiveAlarm
            ? QStringLiteral("color: #d32f2f;")
            : QStringLiteral("color: #2e7d32;"));
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    ui->label_alarmText->setText(localizedAlarmStatusText(status.text.isEmpty()
                                                              ? QStringLiteral("系统正常，无报警")
                                                              : status.text,
                                                          isEnglish));

    if (!errorText.isEmpty())
    {
        qWarning() << "加载报警状态失败:" << errorText;
    }
}

void AlarmWidget::appendAlarmLogRow(int row, const AlarmLogEntry &entry)
{
    if (!ui || !ui->tableWidget_alarmLogs)
    {
        return;
    }

    ui->tableWidget_alarmLogs->setItem(row, 0, new QTableWidgetItem(entry.timestamp.toString("MM-dd HH:mm")));
    ui->tableWidget_alarmLogs->setItem(row, 1, new QTableWidgetItem(entry.type));
    ui->tableWidget_alarmLogs->setItem(row, 2, new QTableWidgetItem(entry.triggerValue.isEmpty() ? QStringLiteral("-") : entry.triggerValue));
    ui->tableWidget_alarmLogs->setItem(row, 3, new QTableWidgetItem(entry.detail));

    const QString typeColor = entry.severity == QStringLiteral("critical")
                                  ? QStringLiteral("#d32f2f")
                                  : QStringLiteral("#f44336");
    if (QTableWidgetItem *typeItem = ui->tableWidget_alarmLogs->item(row, 1))
    {
        typeItem->setForeground(QBrush(QColor(typeColor)));
    }
}

void AlarmWidget::triggerAlarm(const QJsonObject &alarmData)
{
    if (!ui || !ui->tableWidget_alarmLogs)
    {
        return;
    }

    qDebug() << "收到报警:" << alarmData;

    const AlarmLogEntry entry = m_alarmService.fromAlarmData(alarmData);
    ui->tableWidget_alarmLogs->insertRow(0);
    appendAlarmLogRow(0, entry);
    loadAlarmStatus();
    playAlarmAlertTone();
    showAlarmPopup(entry.type, entry.detail);
}

void AlarmWidget::showAlarmPopup(const QString &type, const QString &detail)
{
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    QWidget *dialogParent = QApplication::activeWindow();
    if (!dialogParent)
    {
        dialogParent = window() ? window() : this;
    }
    QMessageBox dialog(QMessageBox::Critical,
                       isEnglish ? QStringLiteral("System Alarm") : QStringLiteral("系统报警"),
                       (isEnglish
                            ? QStringLiteral("Abnormal condition detected!\n\nType: %1\nDetails: %2")
                            : QStringLiteral("检测到异常情况！\n\n报警类型: %1\n详细信息: %2"))
                           .arg(type)
                           .arg(detail),
                       QMessageBox::Ok,
                       dialogParent);
    dialog.setWindowModality(Qt::ApplicationModal);
    dialog.setWindowFlag(Qt::WindowStaysOnTopHint, true);
    QTimer::singleShot(0, &dialog, [&dialog]()
                       {
                           dialog.raise();
                           dialog.activateWindow();
                       });
    dialog.exec();
}

void AlarmWidget::on_btnSaveThresholds_clicked()
{
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    const AlarmThreshold threshold = {
        ui->doubleSpinBox_tempMin->value(),
        ui->doubleSpinBox_tempMax->value(),
        ui->doubleSpinBox_humidityMin->value(),
        ui->doubleSpinBox_humidityMax->value()};

    if (threshold.tempMin >= threshold.tempMax)
    {
        QMessageBox::warning(this,
                             isEnglish ? QStringLiteral("Invalid Parameters") : QStringLiteral("参数错误"),
                             isEnglish ? QStringLiteral("Temperature minimum must be lower than maximum.") : QStringLiteral("温度下限必须小于温度上限。"));
        return;
    }

    if (threshold.humidityMin >= threshold.humidityMax)
    {
        QMessageBox::warning(this,
                             isEnglish ? QStringLiteral("Invalid Parameters") : QStringLiteral("参数错误"),
                             isEnglish ? QStringLiteral("Humidity minimum must be lower than maximum.") : QStringLiteral("湿度下限必须小于湿度上限。"));
        return;
    }

    QString errorText;
    if (!m_alarmService.saveThresholds(threshold, &errorText))
    {
        QMessageBox::critical(this,
                              isEnglish ? QStringLiteral("Save Failed") : QStringLiteral("保存失败"),
                              (isEnglish ? QStringLiteral("Failed to save alarm thresholds:\n") : QStringLiteral("报警阈值保存失败：\n")) + (errorText.isEmpty() ? (isEnglish ? QStringLiteral("Unknown error") : QStringLiteral("未知错误")) : errorText));
        return;
    }

    const QList<QJsonObject> triggeredAlarms = m_alarmService.evaluateLatestEnvironment(&errorText);
    if (!errorText.isEmpty())
    {
        QMessageBox::warning(this,
                             isEnglish ? QStringLiteral("Threshold Saved") : QStringLiteral("阈值已保存"),
                             (isEnglish ? QStringLiteral("Alarm thresholds were saved, but immediate re-evaluation failed:\n") : QStringLiteral("报警阈值已写入数据库，但立即重评估失败：\n")) + errorText);
        refreshData();
        return;
    }

    refreshData();
    for (const QJsonObject &alarmData : triggeredAlarms)
    {
        triggerAlarm(alarmData);
    }

    QMessageBox::information(this,
                             isEnglish ? QStringLiteral("Saved") : QStringLiteral("保存成功"),
                             (isEnglish
                                  ? QStringLiteral("Alarm thresholds have been synchronized to the database.\n\n") + QStringLiteral("Temperature: %1°C - %2°C\n").arg(threshold.tempMin, 0, 'f', 2).arg(threshold.tempMax, 0, 'f', 2) + QStringLiteral("Humidity: %1% - %2%").arg(threshold.humidityMin, 0, 'f', 2).arg(threshold.humidityMax, 0, 'f', 2)
                                  : QStringLiteral("报警阈值已同步到数据库。\n\n") + QStringLiteral("温度范围：%1°C - %2°C\n").arg(threshold.tempMin, 0, 'f', 2).arg(threshold.tempMax, 0, 'f', 2) + QStringLiteral("湿度范围：%1% - %2%").arg(threshold.humidityMin, 0, 'f', 2).arg(threshold.humidityMax, 0, 'f', 2)));
}

void AlarmWidget::on_btnClearLogs_clicked()
{
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        isEnglish ? QStringLiteral("Confirm Clear") : QStringLiteral("确认清空"),
        isEnglish ? QStringLiteral("Are you sure you want to clear all alarm records?") : QStringLiteral("确定要清空所有报警记录吗？"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    QString errorText;
    if (!m_alarmService.clearAlarmLogs(&errorText))
    {
        QMessageBox::critical(this,
                              isEnglish ? QStringLiteral("Clear Failed") : QStringLiteral("清空失败"),
                              (isEnglish ? QStringLiteral("Failed to clear alarm records:\n") : QStringLiteral("报警记录清空失败：\n")) + (errorText.isEmpty() ? (isEnglish ? QStringLiteral("Unknown error") : QStringLiteral("未知错误")) : errorText));
        return;
    }

    refreshData();
    QMessageBox::information(this,
                             isEnglish ? QStringLiteral("Success") : QStringLiteral("成功"),
                             isEnglish ? QStringLiteral("Alarm records were cleared from the database.") : QStringLiteral("报警记录已从数据库清空。"));
}

void AlarmWidget::onAlarmRuntimeDataRefreshed(AlarmStatusSummary status, AlarmLogList logs)
{
    const bool hasActiveAlarm = status.activeCount > 0 || status.level == QStringLiteral("warning") || status.level == QStringLiteral("critical");
    ui->label_alarmIndicator->setText(QStringLiteral("●"));
    ui->label_alarmIndicator->setStyleSheet(
        hasActiveAlarm ? QStringLiteral("color: #d32f2f;") : QStringLiteral("color: #2e7d32;"));
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    ui->label_alarmText->setText(localizedAlarmStatusText(
        status.text.isEmpty() ? QStringLiteral("系统正常，无报警") : status.text, isEnglish));

    ui->tableWidget_alarmLogs->setUpdatesEnabled(false);
    ui->tableWidget_alarmLogs->setRowCount(logs.size());
    for (int i = 0; i < logs.size(); ++i)
    {
        appendAlarmLogRow(i, logs[i]);
    }
    ui->tableWidget_alarmLogs->setUpdatesEnabled(true);

    if (!hasActiveAlarm)
    {
        m_activeAlarmLatched = false;
        m_activeAlarmFingerprint.clear();
        return;
    }

    QString popupType;
    QString popupDetail;
    if (!logs.isEmpty())
    {
        popupType = logs.first().type.trimmed();
        popupDetail = logs.first().detail.trimmed();
    }
    if (popupType.isEmpty())
    {
        popupType = m_languageKey == QStringLiteral("en_US") ? QStringLiteral("System Alarm") : QStringLiteral("系统报警");
    }
    if (popupDetail.isEmpty())
    {
        popupDetail = status.text.trimmed().isEmpty()
                          ? (m_languageKey == QStringLiteral("en_US") ? QStringLiteral("Abnormal condition detected") : QStringLiteral("检测到异常情况"))
                          : status.text.trimmed();
    }

    const QString currentFingerprint = popupType + QStringLiteral("|") + popupDetail;
    const bool shouldNotify = !m_activeAlarmLatched || currentFingerprint != m_activeAlarmFingerprint;
    m_activeAlarmLatched = true;
    m_activeAlarmFingerprint = currentFingerprint;

    if (!shouldNotify)
    {
        return;
    }

    playAlarmAlertTone();
    showAlarmPopup(popupType, popupDetail);
}
