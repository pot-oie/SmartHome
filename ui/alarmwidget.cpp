#include "alarmwidget.h"
#include "ui_alarmwidget.h"
#include <QDebug>
#include <QMessageBox>
#include <QDateTime>

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
    // 加载阈值（假数据）
    ui->doubleSpinBox_tempMax->setValue(30);
    ui->doubleSpinBox_tempMin->setValue(18);
    ui->doubleSpinBox_humidityMax->setValue(70);
    ui->doubleSpinBox_humidityMin->setValue(30);
}

void AlarmWidget::loadFakeAlarmLogs()
{
    // 加载假报警记录
    ui->tableWidget_alarmLogs->setRowCount(0);

    QStringList alarmTypes = {"温度过高", "温度过低", "湿度过高", "PM2.5超标", "CO2浓度过高", "设备离线"};
    QStringList details = {
        "客厅温度达到32.5°C，超过设定上限",
        "卧室温度降至16.2°C，低于设定下限",
        "客厅湿度达到75%，超过设定上限",
        "客厅PM2.5浓度达到85，空气质量较差",
        "卧室CO2浓度达到1200ppm，建议开窗通风",
        "客厅摄像头连接异常，已离线"};

    for (int i = 0; i < 10; i++)
    {
        int row = ui->tableWidget_alarmLogs->rowCount();
        ui->tableWidget_alarmLogs->insertRow(row);

        QDateTime time = QDateTime::currentDateTime().addSecs(-i * 7200);
        int typeIndex = rand() % alarmTypes.size();

        ui->tableWidget_alarmLogs->setItem(row, 0,
                                           new QTableWidgetItem(time.toString("MM-dd hh:mm")));
        ui->tableWidget_alarmLogs->setItem(row, 1,
                                           new QTableWidgetItem(alarmTypes[typeIndex]));

        // 使用图标替代emoji
        QTableWidgetItem *iconItem = new QTableWidgetItem();
        iconItem->setIcon(QIcon(":/icons/warning.svg"));
        ui->tableWidget_alarmLogs->setItem(row, 2, iconItem);

        ui->tableWidget_alarmLogs->setItem(row, 3,
                                           new QTableWidgetItem(details[typeIndex]));

        // 设置报警类型的颜色
        ui->tableWidget_alarmLogs->item(row, 1)->setForeground(QBrush(QColor("#f44336")));
    }
}

void AlarmWidget::triggerAlarm(const QJsonObject &alarmData)
{
    // 触发报警
    qDebug() << "收到报警：" << alarmData;

    // 添加到报警记录
    int row = ui->tableWidget_alarmLogs->rowCount();
    ui->tableWidget_alarmLogs->insertRow(0); // 插入到最前面

    ui->tableWidget_alarmLogs->setItem(0, 0,
                                       new QTableWidgetItem(QDateTime::currentDateTime().toString("MM-dd hh:mm")));
    ui->tableWidget_alarmLogs->setItem(0, 1,
                                       new QTableWidgetItem(alarmData["type"].toString()));

    // 使用图标替代emoji
    QTableWidgetItem *iconItem = new QTableWidgetItem();
    iconItem->setIcon(QIcon(":/icons/warning.svg"));
    ui->tableWidget_alarmLogs->setItem(0, 2, iconItem);

    ui->tableWidget_alarmLogs->setItem(0, 3,
                                       new QTableWidgetItem(alarmData["message"].toString()));

    ui->tableWidget_alarmLogs->item(0, 1)->setForeground(QBrush(QColor("#f44336")));
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
