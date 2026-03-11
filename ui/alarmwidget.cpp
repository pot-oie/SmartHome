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

AlarmWidget::AlarmWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::AlarmWidget)
    , m_refreshTimer(new QTimer(this))
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

    refreshData();
    connect(m_refreshTimer, &QTimer::timeout, this, &AlarmWidget::refreshData);
    m_refreshTimer->start(5000);
}

AlarmWidget::~AlarmWidget()
{
    delete ui;
}

void AlarmWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshData();
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
                    g_alarmTonePlaying = false;
                })
        .detach();
#else
    for (int i = 0; i < 8; ++i)
    {
        QTimer::singleShot(i * 375, this, []()
                           {
                               QApplication::beep();
                           });
    }
#endif
}

void AlarmWidget::refreshData()
{
    loadThresholds();
    loadAlarmStatus();
    loadAlarmLogs();
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
    const bool hasActiveAlarm = status.activeCount > 0
                                || status.level == QStringLiteral("warning")
                                || status.level == QStringLiteral("critical");

    ui->label_alarmIndicator->setText(QStringLiteral("●"));
    ui->label_alarmIndicator->setStyleSheet(
        hasActiveAlarm
            ? QStringLiteral("color: #d32f2f;")
            : QStringLiteral("color: #2e7d32;"));
    ui->label_alarmText->setText(status.text.isEmpty()
                                     ? QStringLiteral("系统正常，无报警")
                                     : status.text);

    if (!errorText.isEmpty())
    {
        qWarning() << "加载报警状态失败:" << errorText;
    }
}

void AlarmWidget::loadAlarmLogs()
{
    ui->tableWidget_alarmLogs->setRowCount(0);

    QString errorText;
    const AlarmLogList logs = m_alarmService.loadAlarmLogs(100, &errorText);
    for (const AlarmLogEntry &entry : logs)
    {
        const int row = ui->tableWidget_alarmLogs->rowCount();
        ui->tableWidget_alarmLogs->insertRow(row);
        appendAlarmLogRow(row, entry);
    }

    if (!errorText.isEmpty())
    {
        qWarning() << "加载报警历史失败:" << errorText;
    }
}

void AlarmWidget::appendAlarmLogRow(int row, const AlarmLogEntry &entry)
{
    ui->tableWidget_alarmLogs->setItem(row, 0, new QTableWidgetItem(entry.timestamp.toString("MM-dd HH:mm")));
    ui->tableWidget_alarmLogs->setItem(row, 1, new QTableWidgetItem(entry.type));
    ui->tableWidget_alarmLogs->setItem(row, 2, new QTableWidgetItem(entry.triggerValue.isEmpty() ? QStringLiteral("-") : entry.triggerValue));
    ui->tableWidget_alarmLogs->setItem(row, 3, new QTableWidgetItem(entry.detail));

    const QString typeColor = entry.severity == QStringLiteral("critical")
                                  ? QStringLiteral("#d32f2f")
                                  : QStringLiteral("#f44336");
    ui->tableWidget_alarmLogs->item(row, 1)->setForeground(QBrush(QColor(typeColor)));
}

void AlarmWidget::triggerAlarm(const QJsonObject &alarmData)
{
    qDebug() << "收到报警:" << alarmData;

    const AlarmLogEntry entry = m_alarmService.fromAlarmData(alarmData);
    ui->tableWidget_alarmLogs->insertRow(0);
    appendAlarmLogRow(0, entry);
    loadAlarmStatus();
    playAlarmAlertTone();

    QMessageBox::critical(this,
                          QStringLiteral("系统报警"),
                          QStringLiteral("检测到异常情况！\n\n报警类型: %1\n详细信息: %2")
                              .arg(entry.type)
                              .arg(entry.detail));
}

void AlarmWidget::on_btnSaveThresholds_clicked()
{
    const AlarmThreshold threshold = {
        ui->doubleSpinBox_tempMin->value(),
        ui->doubleSpinBox_tempMax->value(),
        ui->doubleSpinBox_humidityMin->value(),
        ui->doubleSpinBox_humidityMax->value()};

    if (threshold.tempMin >= threshold.tempMax)
    {
        QMessageBox::warning(this,
                             QStringLiteral("参数错误"),
                             QStringLiteral("温度下限必须小于温度上限。"));
        return;
    }

    if (threshold.humidityMin >= threshold.humidityMax)
    {
        QMessageBox::warning(this,
                             QStringLiteral("参数错误"),
                             QStringLiteral("湿度下限必须小于湿度上限。"));
        return;
    }

    QString errorText;
    if (!m_alarmService.saveThresholds(threshold, &errorText))
    {
        QMessageBox::critical(this,
                              QStringLiteral("保存失败"),
                              QStringLiteral("报警阈值保存失败：\n")
                                  + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
        return;
    }

    const QList<QJsonObject> triggeredAlarms = m_alarmService.evaluateLatestEnvironment(&errorText);
    if (!errorText.isEmpty())
    {
        QMessageBox::warning(this,
                             QStringLiteral("阈值已保存"),
                             QStringLiteral("报警阈值已写入数据库，但立即重评估失败：\n") + errorText);
        refreshData();
        return;
    }

    refreshData();
    for (const QJsonObject &alarmData : triggeredAlarms)
    {
        triggerAlarm(alarmData);
    }

    QMessageBox::information(this,
                             QStringLiteral("保存成功"),
                             QStringLiteral("报警阈值已同步到数据库。\n\n")
                                 + QStringLiteral("温度范围：%1°C - %2°C\n")
                                       .arg(threshold.tempMin, 0, 'f', 2)
                                       .arg(threshold.tempMax, 0, 'f', 2)
                                 + QStringLiteral("湿度范围：%1%% - %2%%")
                                       .arg(threshold.humidityMin, 0, 'f', 2)
                                       .arg(threshold.humidityMax, 0, 'f', 2));
}

void AlarmWidget::on_btnClearLogs_clicked()
{
    const QMessageBox::StandardButton reply = QMessageBox::question(
        this,
        QStringLiteral("确认清空"),
        QStringLiteral("确定要清空所有报警记录吗？"),
        QMessageBox::Yes | QMessageBox::No);
    if (reply != QMessageBox::Yes)
    {
        return;
    }

    QString errorText;
    if (!m_alarmService.clearAlarmLogs(&errorText))
    {
        QMessageBox::critical(this,
                              QStringLiteral("清空失败"),
                              QStringLiteral("报警记录清空失败：\n")
                                  + (errorText.isEmpty() ? QStringLiteral("未知错误") : errorText));
        return;
    }

    refreshData();
    QMessageBox::information(this, QStringLiteral("成功"), QStringLiteral("报警记录已从数据库清空。"));
}
