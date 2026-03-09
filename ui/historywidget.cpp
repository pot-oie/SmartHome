#include "historywidget.h"
#include "ui_historywidget.h"
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>

HistoryWidget::HistoryWidget(QWidget *parent) : QWidget(parent),
                                                ui(new Ui::HistoryWidget)
{
    ui->setupUi(this);

    // 初始化操作日志表格
    ui->tableWidget_logs->setColumnCount(5);
    ui->tableWidget_logs->setHorizontalHeaderLabels(
        {"时间", "用户", "操作类型", "设备", "详情"});
    ui->tableWidget_logs->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_logs->setAlternatingRowColors(true);
    ui->tableWidget_logs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_logs->setSelectionBehavior(QAbstractItemView::SelectRows);

    // 加载假数据
    queryOperationLogs();

    // 设置日期范围
    ui->dateTimeEdit_start->setDate(QDate::currentDate().addDays(-7));
    ui->dateTimeEdit_end->setDate(QDate::currentDate());
}

HistoryWidget::~HistoryWidget()
{
    delete ui;
}

void HistoryWidget::queryOperationLogs()
{
    // 查询日志逻辑（假数据）
    ui->tableWidget_logs->setRowCount(0);

    // 生成假数据
    QStringList users = {"admin", "张三", "李四"};
    QStringList operations = {"设备控制", "场景激活", "参数调节", "系统设置"};
    QStringList devices = {"客厅主灯", "客厅空调", "卧室窗帘", "智能门锁", "客厅电视"};
    QStringList details = {
        "开启设备", "关闭设备", "调节温度至24°C",
        "调节亮度至80%", "激活回家模式", "修改报警阈值"};

    for (int i = 0; i < 20; i++)
    {
        int row = ui->tableWidget_logs->rowCount();
        ui->tableWidget_logs->insertRow(row);

        // 时间（倒序）
        QDateTime time = QDateTime::currentDateTime().addSecs(-i * 3600);
        ui->tableWidget_logs->setItem(row, 0,
                                      new QTableWidgetItem(time.toString("yyyy-MM-dd hh:mm:ss")));

        // 用户
        ui->tableWidget_logs->setItem(row, 1,
                                      new QTableWidgetItem(users[rand() % users.size()]));

        // 操作类型
        ui->tableWidget_logs->setItem(row, 2,
                                      new QTableWidgetItem(operations[rand() % operations.size()]));

        // 设备
        ui->tableWidget_logs->setItem(row, 3,
                                      new QTableWidgetItem(devices[rand() % devices.size()]));

        // 详情
        ui->tableWidget_logs->setItem(row, 4,
                                      new QTableWidgetItem(details[rand() % details.size()]));
    }
}

void HistoryWidget::queryEnvironmentDataAndDrawChart()
{
    // 查询数据绘图逻辑
    // TODO: 使用 QCustomPlot 绘制温湿度折线图
    qDebug() << "绘制环境数据图表";
}

void HistoryWidget::on_btnSearch_clicked()
{
    qDebug() << "执行查询";
    qDebug() << "查询时间范围：" << ui->dateTimeEdit_start->date() << " 到 " << ui->dateTimeEdit_end->date();
    queryOperationLogs();
    QMessageBox::information(this, "查询完成", "已加载操作日志记录！");
}

void HistoryWidget::on_btnExport_clicked()
{
    qDebug() << "执行导出";

    QString fileName = QFileDialog::getSaveFileName(this, "导出数据",
                                                    QDir::homePath() + "/操作日志.csv",
                                                    "CSV文件 (*.csv)");
    if (fileName.isEmpty())
    {
        return;
    }

    QMessageBox::information(this, "导出成功",
                             "数据已导出到：\n" + fileName + "\n\n" +
                                 "（演示版本，实际未生成文件）");
}

void HistoryWidget::on_tabWidget_currentChanged(int index)
{
    qDebug() << "Tab页切换：" << index;
    if (index == 1)
    {
        // 切换到环境数据图表页
        queryEnvironmentDataAndDrawChart();
    }
}
