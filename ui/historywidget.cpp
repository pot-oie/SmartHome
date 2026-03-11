#include "historywidget.h"
#include "ui_historywidget.h"

#include "qcustomplot.h"

#include <QtConcurrent>
#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QTableWidgetItem>

namespace
{
    const QString kHint = QStringLiteral("提示");
    const QString kFailed = QStringLiteral("失败");
    const QString kSuccess = QStringLiteral("成功");
}

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::HistoryWidget), m_logWatcher(new QFutureWatcher<OperationLogList>(this)), m_envSeriesWatcher(new QFutureWatcher<EnvironmentSeries>(this))
{
    ui->setupUi(this);

    ui->tableWidget_logs->setColumnCount(6);
    ui->tableWidget_logs->setHorizontalHeaderLabels(
        {QStringLiteral("时间"),
         QStringLiteral("用户"),
         QStringLiteral("操作类型"),
         QStringLiteral("设备"),
         QStringLiteral("详情"),
         QStringLiteral("结果")});
    ui->tableWidget_logs->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_logs->setAlternatingRowColors(true);
    ui->tableWidget_logs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_logs->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_logs->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->comboBox_deviceType->clear();
    ui->comboBox_deviceType->addItem(QStringLiteral("全部"));
    const QStringList categories = m_deviceService.categories();
    for (int index = 1; index < categories.size(); ++index)
    {
        ui->comboBox_deviceType->addItem(categories.at(index));
    }

    ui->dateTimeEdit_start->setDateTime(QDateTime(QDate::currentDate().addDays(-7), QTime(0, 0, 0)));
    ui->dateTimeEdit_end->setDateTime(QDateTime(QDate::currentDate(), QTime(23, 59, 59)));

    const auto normalizeStartDateTime = [this]()
    {
        const QDateTime normalizedDateTime(ui->dateTimeEdit_start->date(), QTime(0, 0, 0));
        if (ui->dateTimeEdit_start->dateTime() == normalizedDateTime)
        {
            return;
        }

        const QSignalBlocker blocker(ui->dateTimeEdit_start);
        ui->dateTimeEdit_start->setDateTime(normalizedDateTime);
    };

    const auto normalizeEndDateTime = [this]()
    {
        const QDateTime normalizedDateTime(ui->dateTimeEdit_end->date(), QTime(23, 59, 59));
        if (ui->dateTimeEdit_end->dateTime() == normalizedDateTime)
        {
            return;
        }

        const QSignalBlocker blocker(ui->dateTimeEdit_end);
        ui->dateTimeEdit_end->setDateTime(normalizedDateTime);
    };

    connect(ui->dateTimeEdit_start, &QDateTimeEdit::dateChanged, this, [normalizeStartDateTime](const QDate &) { normalizeStartDateTime(); });
    connect(ui->dateTimeEdit_start, &QDateTimeEdit::timeChanged, this, [normalizeStartDateTime](const QTime &) { normalizeStartDateTime(); });
    connect(ui->dateTimeEdit_end, &QDateTimeEdit::dateChanged, this, [normalizeEndDateTime](const QDate &) { normalizeEndDateTime(); });
    connect(ui->dateTimeEdit_end, &QDateTimeEdit::timeChanged, this, [normalizeEndDateTime](const QTime &) { normalizeEndDateTime(); });

    m_btnDeleteLog = new QPushButton(QStringLiteral("删除选中日志"), this);
    if (QLayout *filterLayout = ui->groupBox_filter->layout())
    {
        filterLayout->addWidget(m_btnDeleteLog);
    }

    connect(m_btnDeleteLog, &QPushButton::clicked, this, &HistoryWidget::deleteSelectedOperationLog);
    connect(m_logWatcher, &QFutureWatcher<OperationLogList>::finished, this, &HistoryWidget::onOperationLogsLoaded);
    connect(m_envSeriesWatcher, &QFutureWatcher<EnvironmentSeries>::finished, this, &HistoryWidget::onEnvironmentSeriesLoaded);

    queryOperationLogs();
}

HistoryWidget::~HistoryWidget()
{
    delete ui;
}

void HistoryWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshData();
}

void HistoryWidget::refreshData()
{
    queryOperationLogs();
    if (ui->tabWidget->currentIndex() == 1)
    {
        queryEnvironmentDataAndDrawChart();
    }
}

void HistoryWidget::renderOperationLogs(const OperationLogList &logs)
{
    ui->tableWidget_logs->setRowCount(0);
    for (const OperationLogEntry &entry : logs)
    {
        const int row = ui->tableWidget_logs->rowCount();
        ui->tableWidget_logs->insertRow(row);

        QTableWidgetItem *timeItem = new QTableWidgetItem(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
        timeItem->setData(Qt::UserRole, entry.recordId);
        ui->tableWidget_logs->setItem(row, 0, timeItem);
        ui->tableWidget_logs->setItem(row, 1, new QTableWidgetItem(entry.user));
        ui->tableWidget_logs->setItem(row, 2, new QTableWidgetItem(entry.operation));
        ui->tableWidget_logs->setItem(row, 3, new QTableWidgetItem(entry.device));
        ui->tableWidget_logs->setItem(row, 4, new QTableWidgetItem(entry.detail));
        ui->tableWidget_logs->setItem(row, 5, new QTableWidgetItem(entry.result));
    }
}

void HistoryWidget::queryOperationLogs()
{
    if (m_logWatcher->isRunning())
    {
        return;
    }

    const QDate startDate = ui->dateTimeEdit_start->date();
    const QDate endDate = ui->dateTimeEdit_end->date();
    const QDateTime startTime(startDate, QTime(0, 0, 0));
    const QDateTime endTime(endDate.addDays(1), QTime(0, 0, 0));
    const QString deviceType = ui->comboBox_deviceType->currentText();

    ui->tableWidget_logs->setRowCount(0);
    m_logWatcher->setProperty("requestId", ++m_logRequestId);
    m_logWatcher->setFuture(QtConcurrent::run([startTime, endTime, deviceType]()
                                              {
        HistoryService historyService;
        return historyService.queryOperationLogs(startTime, endTime, deviceType); }));
}

void HistoryWidget::renderEnvironmentSeries(const EnvironmentSeries &series)
{
    ui->label_chartPlaceholder->hide();

    if (!customPlot)
    {
        customPlot = new QCustomPlot(this);
        ui->verticalLayout_charts->addWidget(customPlot);

        customPlot->addGraph();
        customPlot->graph(0)->setPen(QPen(Qt::red, 2));
        customPlot->graph(0)->setName(QStringLiteral("温度 (C)"));

        customPlot->addGraph();
        customPlot->graph(1)->setPen(QPen(Qt::blue, 2));
        customPlot->graph(1)->setName(QStringLiteral("湿度 (%)"));

        QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
        dateTicker->setDateTimeFormat("MM-dd hh:mm");
        customPlot->xAxis->setTicker(dateTicker);
        customPlot->xAxis->setLabel(QStringLiteral("时间"));
        customPlot->yAxis->setLabel(QStringLiteral("环境数值"));
        customPlot->legend->setVisible(true);
    }

    QVector<double> timeData;
    QVector<double> tempData;
    QVector<double> humData;
    for (const EnvironmentPoint &point : series)
    {
        timeData.push_back(point.timestamp.toSecsSinceEpoch());
        tempData.push_back(point.temperature);
        humData.push_back(point.humidity);
    }

    if (timeData.isEmpty())
    {
        ui->label_chartPlaceholder->setText(QStringLiteral("当前没有可用的环境历史数据。"));
        ui->label_chartPlaceholder->show();
        return;
    }

    customPlot->graph(0)->setData(timeData, tempData);
    customPlot->graph(1)->setData(timeData, humData);
    customPlot->xAxis->setRange(timeData.first(), timeData.last());
    customPlot->yAxis->setRange(0, 100);
    customPlot->replot();
}

void HistoryWidget::queryEnvironmentDataAndDrawChart()
{
    if (m_envSeriesWatcher->isRunning())
    {
        return;
    }

    ui->label_chartPlaceholder->setText(QStringLiteral("正在加载环境历史数据..."));
    ui->label_chartPlaceholder->show();
    m_envSeriesWatcher->setProperty("requestId", ++m_envSeriesRequestId);
    m_envSeriesWatcher->setFuture(QtConcurrent::run([]()
                                                    {
        HistoryService historyService;
        return historyService.queryEnvironmentSeries(24); }));
}

void HistoryWidget::onOperationLogsLoaded()
{
    if (m_logWatcher->property("requestId").toInt() != m_logRequestId)
    {
        return;
    }

    m_currentLogs = m_logWatcher->result();
    renderOperationLogs(m_currentLogs);
    if (m_showLogLoadedMessage)
    {
        m_showLogLoadedMessage = false;
        QMessageBox::information(this, kSuccess, QStringLiteral("已加载 %1 条操作日志记录。").arg(m_currentLogs.size()));
    }
}

void HistoryWidget::onEnvironmentSeriesLoaded()
{
    if (m_envSeriesWatcher->property("requestId").toInt() != m_envSeriesRequestId)
    {
        return;
    }

    renderEnvironmentSeries(m_envSeriesWatcher->result());
}

void HistoryWidget::on_btnSearch_clicked()
{
    if (ui->dateTimeEdit_start->date() > ui->dateTimeEdit_end->date())
    {
        QMessageBox::warning(this, kFailed, QStringLiteral("开始时间不能晚于结束时间。"));
        return;
    }

    m_showLogLoadedMessage = true;
    queryOperationLogs();
}

void HistoryWidget::on_btnExport_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("导出数据"),
        QDir::homePath() + "/operation_logs.xlsx",
        "Excel Files (*.xlsx)");
    if (fileName.isEmpty())
    {
        return;
    }

    if (QFileInfo(fileName).suffix().isEmpty())
    {
        fileName += QStringLiteral(".xlsx");
    }

    if (m_currentLogs.isEmpty())
    {
        QMessageBox::warning(this, kFailed, QStringLiteral("当前没有可导出的日志数据。"));
        return;
    }

    QString errorMessage;
    if (!m_historyService.exportOperationLogsToExcel(fileName, m_currentLogs, &errorMessage))
    {
        QMessageBox::critical(this, kFailed, QStringLiteral("导出失败：") + errorMessage);
        return;
    }

    QMessageBox::information(this, kSuccess, QStringLiteral("数据已导出到：\n") + fileName);
}

void HistoryWidget::on_tabWidget_currentChanged(int index)
{
    if (index == 0)
    {
        queryOperationLogs();
    }
    else if (index == 1)
    {
        queryEnvironmentDataAndDrawChart();
    }
}

void HistoryWidget::deleteSelectedOperationLog()
{
    const int currentRow = ui->tableWidget_logs->currentRow();
    if (currentRow < 0 || currentRow >= m_currentLogs.size())
    {
        QMessageBox::warning(this, kHint, QStringLiteral("请先选择一条日志记录。"));
        return;
    }

    const OperationLogEntry entry = m_currentLogs.at(currentRow);
    if (QMessageBox::question(this,
                              QStringLiteral("确认删除"),
                              QStringLiteral("确定要删除该条历史记录吗？")) != QMessageBox::Yes)
    {
        return;
    }

    QString errorMessage;
    if (!m_historyService.deleteOperationLog(entry.recordId, &errorMessage))
    {
        QMessageBox::critical(this, kFailed, QStringLiteral("删除失败：") + errorMessage);
        return;
    }

    queryOperationLogs();
}
