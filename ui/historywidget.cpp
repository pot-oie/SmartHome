#include "historywidget.h"
#include "ui_historywidget.h"
#include "qcustomplot.h"
#include <QDebug>
#include <QDateTime>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>

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

    // 设置日期范围
    ui->dateTimeEdit_start->setDate(QDate::currentDate().addDays(-7));
    ui->dateTimeEdit_end->setDate(QDate::currentDate());

    // 初次加载日志
    queryOperationLogs();
}

HistoryWidget::~HistoryWidget()
{
    delete ui;
}

void HistoryWidget::queryOperationLogs()
{
    const QDate startDate = ui->dateTimeEdit_start->date();
    const QDate endDate = ui->dateTimeEdit_end->date();

    m_currentLogs = m_historyService.queryOperationLogs(startDate, endDate);

    ui->tableWidget_logs->setRowCount(0);
    for (const OperationLogEntry &entry : m_currentLogs)
    {
        int row = ui->tableWidget_logs->rowCount();
        ui->tableWidget_logs->insertRow(row);

        ui->tableWidget_logs->setItem(row, 0, new QTableWidgetItem(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss")));
        ui->tableWidget_logs->setItem(row, 1, new QTableWidgetItem(entry.user));
        ui->tableWidget_logs->setItem(row, 2, new QTableWidgetItem(entry.operation));
        ui->tableWidget_logs->setItem(row, 3, new QTableWidgetItem(entry.device));
        ui->tableWidget_logs->setItem(row, 4, new QTableWidgetItem(entry.detail));
    }
}

void HistoryWidget::queryEnvironmentDataAndDrawChart()
{
    qDebug() << "绘制环境数据图表";

    // 1. 隐藏原来的占位提示文本
    ui->label_chartPlaceholder->hide();

    // 2. 如果图表尚未创建，则在代码中动态初始化
    if (!customPlot)
    {
        customPlot = new QCustomPlot(this);
        // 将 QCustomPlot 添加到由 Designer 生成的垂直布局中
        ui->verticalLayout_charts->addWidget(customPlot);

        // 初始化温度曲线 (红色)
        customPlot->addGraph();
        customPlot->graph(0)->setPen(QPen(Qt::red, 2));
        customPlot->graph(0)->setName("温度 (°C)");

        // 初始化湿度曲线 (蓝色)
        customPlot->addGraph();
        customPlot->graph(1)->setPen(QPen(Qt::blue, 2));
        customPlot->graph(1)->setName("湿度 (%)");

        // 设置时间轴(X轴)
        QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
        dateTicker->setDateTimeFormat("hh:mm:ss");
        customPlot->xAxis->setTicker(dateTicker);
        customPlot->xAxis->setLabel("时间");

        // 设置数值轴(Y轴)
        customPlot->yAxis->setLabel("环境数值");
        customPlot->legend->setVisible(true); // 显示图例
    }

    // 3. 通过 service 查询最近 24 个时间点的温湿度数据
    QVector<double> timeData, tempData, humData;
    const EnvironmentSeries series = m_historyService.queryEnvironmentSeries(24);
    for (const EnvironmentPoint &point : series)
    {
        timeData.push_back(point.timestamp.toSecsSinceEpoch());
        tempData.push_back(point.temperature);
        humData.push_back(point.humidity);
    }

    if (timeData.isEmpty())
    {
        return;
    }

    // 4. 将数据赋予图表
    customPlot->graph(0)->setData(timeData, tempData);
    customPlot->graph(1)->setData(timeData, humData);

    // 5. 自动缩放坐标轴范围以适应数据
    customPlot->xAxis->setRange(timeData.first(), timeData.last());
    // 假设温度一般10~40，湿度30~80，给Y轴一个固定的舒适范围
    customPlot->yAxis->setRange(10, 85);

    // 6. 重绘刷新显示
    customPlot->replot();
}

void HistoryWidget::on_btnSearch_clicked()
{
    qDebug() << "执行查询";
    qDebug() << "查询时间范围：" << ui->dateTimeEdit_start->date() << " 到 " << ui->dateTimeEdit_end->date();

    if (ui->dateTimeEdit_start->date() > ui->dateTimeEdit_end->date())
    {
        QMessageBox::warning(this, "查询失败", "开始时间不能晚于结束时间。");
        return;
    }

    queryOperationLogs();
    QMessageBox::information(this, "查询完成", QString("已加载 %1 条操作日志记录！").arg(m_currentLogs.size()));
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

    if (m_currentLogs.isEmpty())
    {
        QMessageBox::warning(this, "导出失败", "当前没有可导出的日志数据，请先查询。");
        return;
    }

    QString errorMessage;
    if (!m_historyService.exportOperationLogsToCsv(fileName, m_currentLogs, &errorMessage))
    {
        QMessageBox::critical(this, "导出失败", "文件写入失败：" + errorMessage);
        return;
    }

    QMessageBox::information(this, "导出成功", "数据已导出到：\n" + fileName);
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
