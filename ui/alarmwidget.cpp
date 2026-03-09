#include "alarmwidget.h"
#include "ui_alarmwidget.h"

#include <QDebug>
#include <QMessageBox>

AlarmWidget::AlarmWidget(QWidget *parent) : QWidget(parent),
                                            ui(new Ui::AlarmWidget)
{
    ui->setupUi(this);
    loadThresholds();

    // 初始化报警记录表格
    ui->tableWidget_alarmLogs->setColumnCount(4);
    ui->tableWidget_alarmLogs->setHorizontalHeaderLabels(
        {"时间", "报警类型", "触发值", "详情"});
    ui->tableWidget_alarmLogs->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_alarmLogs->setAlternatingRowColors(true);
    ui->tableWidget_alarmLogs->setEditTriggers(QAbstractItemView::NoEditTriggers);

    // 加载假报警记录
    loadFakeAlarmLogs();
}

AlarmWidget::~AlarmWidget()
{
    delete ui;
}

void AlarmWidget::loadThresholds()
{
    const AlarmThreshold threshold = m_alarmService.defaultThreshold();
    ui->doubleSpinBox_tempMax->setValue(threshold.tempMax);
    ui->doubleSpinBox_tempMin->setValue(threshold.tempMin);
    ui->doubleSpinBox_humidityMax->setValue(threshold.humidityMax);
    ui->doubleSpinBox_humidityMin->setValue(threshold.humidityMin);
}

void AlarmWidget::loadFakeAlarmLogs()
{
    ui->tableWidget_alarmLogs->setRowCount(0);

    const AlarmLogList logs = m_alarmService.loadSampleLogs(10);
    for (const AlarmLogEntry &entry : logs)
    {
        const int row = ui->tableWidget_alarmLogs->rowCount();
        ui->tableWidget_alarmLogs->insertRow(row);
        appendAlarmLogRow(row, entry);
    }
}

void AlarmWidget::appendAlarmLogRow(int row, const AlarmLogEntry &entry)
{
    ui->tableWidget_alarmLogs->setItem(row, 0, new QTableWidgetItem(entry.timestamp.toString("MM-dd hh:mm")));
    ui->tableWidget_alarmLogs->setItem(row, 1, new QTableWidgetItem(entry.type));

    QTableWidgetItem *iconItem = new QTableWidgetItem();
    iconItem->setIcon(QIcon(":/icons/warning.svg"));
    ui->tableWidget_alarmLogs->setItem(row, 2, iconItem);

    ui->tableWidget_alarmLogs->setItem(row, 3, new QTableWidgetItem(entry.detail));
    ui->tableWidget_alarmLogs->item(row, 1)->setForeground(QBrush(QColor("#f44336")));
}

void AlarmWidget::triggerAlarm(const QJsonObject &alarmData)
{
    qDebug() << "收到报警：" << alarmData;

    const AlarmLogEntry entry = m_alarmService.fromAlarmData(alarmData);
    ui->tableWidget_alarmLogs->insertRow(0);
    appendAlarmLogRow(0, entry);

    QMessageBox::critical(this, "系统报警",
                          QString("检测到异常情况！\n\n报警类型: %1\n详细信息: %2")
                              .arg(entry.type)
                              .arg(entry.detail));
}

void AlarmWidget::on_btnSaveThresholds_clicked()
{
    qDebug() << "保存阈值";

    QMessageBox::information(this, "保存成功",
                             QString("报警阈值已保存：\n\n") +
                                 QString("温度范围：%1°C - %2°C\n").arg(ui->doubleSpinBox_tempMin->value()).arg(ui->doubleSpinBox_tempMax->value()) +
                                 QString("湿度范围：%1% - %2%").arg(ui->doubleSpinBox_humidityMin->value()).arg(ui->doubleSpinBox_humidityMax->value()));
}

void AlarmWidget::on_btnClearLogs_clicked()
{
    qDebug() << "清除日志";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "确认清除",
                                  "确定要清除所有报警记录吗？",
                                  QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes)
    {
        ui->tableWidget_alarmLogs->setRowCount(0);
        QMessageBox::information(this, "成功", "报警记录已清除！");
    }
}
