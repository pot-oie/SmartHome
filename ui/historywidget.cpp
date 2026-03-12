#include "historywidget.h"
#include "ui_historywidget.h"

#include "qcustomplot.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
#include <QHBoxLayout>
#include <QLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QTableWidgetItem>

namespace
{
    const QString kHint = QStringLiteral("\u63d0\u793a");
    const QString kFailed = QStringLiteral("\u5931\u8d25");
    const QString kSuccess = QStringLiteral("\u6210\u529f");

    QString localizedText(const QString &value, bool isEnglish)
    {
        if (!isEnglish)
        {
            return value;
        }

        static const QHash<QString, QString> map = {
            {QStringLiteral("管理员"), QStringLiteral("Admin")},
            {QStringLiteral("开启"), QStringLiteral("On")},
            {QStringLiteral("关闭"), QStringLiteral("Off")},
            {QStringLiteral("成功"), QStringLiteral("Success")},
            {QStringLiteral("失败"), QStringLiteral("Failed")},
            {QStringLiteral("客厅主灯"), QStringLiteral("Living Room Light")},
            {QStringLiteral("卧室灯"), QStringLiteral("Bedroom Light")},
            {QStringLiteral("客厅空调"), QStringLiteral("Living Room AC")},
            {QStringLiteral("客厅窗帘"), QStringLiteral("Living Room Curtain")},
            {QStringLiteral("前门智能锁"), QStringLiteral("Front Door Lock")},
            {QStringLiteral("客厅摄像头"), QStringLiteral("Living Room Camera")},
            {QStringLiteral("客厅电视"), QStringLiteral("Living Room TV")}};
        return map.value(value, value);
    }
}

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HistoryWidget)
{
    ui->setupUi(this);

    ui->tableWidget_logs->setColumnCount(6);
    ui->tableWidget_logs->setHorizontalHeaderLabels(
        {QStringLiteral("\u65f6\u95f4"),
         QStringLiteral("\u7528\u6237"),
         QStringLiteral("\u64cd\u4f5c\u7c7b\u578b"),
         QStringLiteral("\u8bbe\u5907"),
         QStringLiteral("\u8be6\u60c5"),
         QStringLiteral("\u7ed3\u679c")});
    ui->tableWidget_logs->horizontalHeader()->setStretchLastSection(true);
    ui->tableWidget_logs->setAlternatingRowColors(true);
    ui->tableWidget_logs->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget_logs->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableWidget_logs->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->comboBox_deviceType->clear();
    ui->comboBox_deviceType->addItem(QStringLiteral("\u5168\u90e8"));
    QStringList categories = m_deviceService.categories();
    for (int index = 1; index < categories.size(); ++index)
    {
        ui->comboBox_deviceType->addItem(categories.at(index));
    }

    ui->dateTimeEdit_start->setDate(QDate::currentDate().addDays(-7));
    ui->dateTimeEdit_end->setDate(QDate::currentDate());

    m_btnDeleteLog = new QPushButton(QStringLiteral("\u5220\u9664\u9009\u4e2d\u65e5\u5fd7"), this);
    if (QLayout *filterLayout = ui->groupBox_filter->layout())
    {
        filterLayout->addWidget(m_btnDeleteLog);
    }
    connect(m_btnDeleteLog, &QPushButton::clicked, this, &HistoryWidget::deleteSelectedOperationLog);

    queryOperationLogs();
    applyLanguage(QStringLiteral("zh_CN"));
}

HistoryWidget::~HistoryWidget()
{
    delete ui;
}

void HistoryWidget::applyLanguage(const QString &languageKey)
{
    if (languageKey.trimmed().isEmpty())
    {
        return;
    }

    m_languageKey = languageKey;
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    ui->groupBox_filter->setTitle(isEnglish ? QStringLiteral("Filters") : QStringLiteral("查询条件"));
    ui->label_startTime->setText(isEnglish ? QStringLiteral("Start Time:") : QStringLiteral("开始时间:"));
    ui->label_endTime->setText(isEnglish ? QStringLiteral("End Time:") : QStringLiteral("结束时间:"));
    ui->label_deviceType->setText(isEnglish ? QStringLiteral("Device Type:") : QStringLiteral("设备类型:"));
    ui->btnSearch->setText(isEnglish ? QStringLiteral("Search") : QStringLiteral("查询记录"));
    ui->btnExport->setText(isEnglish ? QStringLiteral("Export Excel") : QStringLiteral("导出Excel"));
    if (m_btnDeleteLog)
    {
        m_btnDeleteLog->setText(isEnglish ? QStringLiteral("Delete Selected") : QStringLiteral("删除选中日志"));
    }

    ui->tabWidget->setTabText(0, isEnglish ? QStringLiteral("Operation Logs") : QStringLiteral("操作日志列表"));
    ui->tabWidget->setTabText(1, isEnglish ? QStringLiteral("Environment Chart") : QStringLiteral("环境数据折线图"));
    ui->tableWidget_logs->setHorizontalHeaderLabels(isEnglish
                                                        ? QStringList{QStringLiteral("Time"), QStringLiteral("User"), QStringLiteral("Operation"), QStringLiteral("Device"), QStringLiteral("Details"), QStringLiteral("Result")}
                                                        : QStringList{QStringLiteral("时间"), QStringLiteral("用户"), QStringLiteral("操作类型"), QStringLiteral("设备"), QStringLiteral("详情"), QStringLiteral("结果")});
}

void HistoryWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    queryOperationLogs();
    if (ui->tabWidget->currentIndex() == 1)
    {
        queryEnvironmentDataAndDrawChart();
    }
}

void HistoryWidget::queryOperationLogs()
{
    const QDate startDate = ui->dateTimeEdit_start->date();
    const QDate endDate = ui->dateTimeEdit_end->date();
    const QString deviceType = ui->comboBox_deviceType->currentText();

    m_currentLogs = m_historyService.queryOperationLogs(startDate, endDate, deviceType);

    ui->tableWidget_logs->setRowCount(0);
    for (const OperationLogEntry &entry : m_currentLogs)
    {
        const int row = ui->tableWidget_logs->rowCount();
        ui->tableWidget_logs->insertRow(row);

        QTableWidgetItem *timeItem = new QTableWidgetItem(entry.timestamp.toString("yyyy-MM-dd hh:mm:ss"));
        timeItem->setData(Qt::UserRole, entry.recordId);
        ui->tableWidget_logs->setItem(row, 0, timeItem);
        const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
        ui->tableWidget_logs->setItem(row, 1, new QTableWidgetItem(localizedText(entry.user, isEnglish)));
        ui->tableWidget_logs->setItem(row, 2, new QTableWidgetItem(localizedText(entry.operation, isEnglish)));
        ui->tableWidget_logs->setItem(row, 3, new QTableWidgetItem(localizedText(entry.device, isEnglish)));
        ui->tableWidget_logs->setItem(row, 4, new QTableWidgetItem(isEnglish ? entry.detail : entry.detail));
        ui->tableWidget_logs->setItem(row, 5, new QTableWidgetItem(localizedText(entry.result, isEnglish)));
    }
}

void HistoryWidget::queryEnvironmentDataAndDrawChart()
{
    ui->label_chartPlaceholder->hide();

    if (!customPlot)
    {
        customPlot = new QCustomPlot(this);
        ui->verticalLayout_charts->addWidget(customPlot);

        customPlot->addGraph();
        customPlot->graph(0)->setPen(QPen(Qt::red, 2));
        customPlot->graph(0)->setName(QStringLiteral("\u6e29\u5ea6 (C)"));

        customPlot->addGraph();
        customPlot->graph(1)->setPen(QPen(Qt::blue, 2));
        customPlot->graph(1)->setName(QStringLiteral("\u6e7f\u5ea6 (%)"));

        QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
        dateTicker->setDateTimeFormat("MM-dd hh:mm");
        customPlot->xAxis->setTicker(dateTicker);
        customPlot->xAxis->setLabel(QStringLiteral("\u65f6\u95f4"));
        customPlot->yAxis->setLabel(QStringLiteral("\u73af\u5883\u6570\u503c"));
        customPlot->legend->setVisible(true);
    }

    QVector<double> timeData;
    QVector<double> tempData;
    QVector<double> humData;
    const EnvironmentSeries series = m_historyService.queryEnvironmentSeries(24);
    for (const EnvironmentPoint &point : series)
    {
        timeData.push_back(point.timestamp.toSecsSinceEpoch());
        tempData.push_back(point.temperature);
        humData.push_back(point.humidity);
    }

    if (timeData.isEmpty())
    {
        ui->label_chartPlaceholder->setText(QStringLiteral("\u5f53\u524d\u6ca1\u6709\u53ef\u7528\u7684\u73af\u5883\u5386\u53f2\u6570\u636e\u3002"));
        ui->label_chartPlaceholder->show();
        return;
    }

    customPlot->graph(0)->setData(timeData, tempData);
    customPlot->graph(1)->setData(timeData, humData);
    customPlot->xAxis->setRange(timeData.first(), timeData.last());
    customPlot->yAxis->setRange(0, 100);
    customPlot->replot();
}

void HistoryWidget::on_btnSearch_clicked()
{
    if (ui->dateTimeEdit_start->date() > ui->dateTimeEdit_end->date())
    {
        QMessageBox::warning(this, kFailed, QStringLiteral("\u5f00\u59cb\u65f6\u95f4\u4e0d\u80fd\u665a\u4e8e\u7ed3\u675f\u65f6\u95f4\u3002"));
        return;
    }

    queryOperationLogs();
    QMessageBox::information(this, kSuccess, QStringLiteral("\u5df2\u52a0\u8f7d %1 \u6761\u64cd\u4f5c\u65e5\u5fd7\u8bb0\u5f55\u3002").arg(m_currentLogs.size()));
}

void HistoryWidget::on_btnExport_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        QStringLiteral("\u5bfc\u51fa\u6570\u636e"),
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
        QMessageBox::warning(this, kFailed, QStringLiteral("\u5f53\u524d\u6ca1\u6709\u53ef\u5bfc\u51fa\u7684\u65e5\u5fd7\u6570\u636e\u3002"));
        return;
    }

    QString errorMessage;
    if (!m_historyService.exportOperationLogsToExcel(fileName, m_currentLogs, &errorMessage))
    {
        QMessageBox::critical(this, kFailed, QStringLiteral("\u5bfc\u51fa\u5931\u8d25\uff1a") + errorMessage);
        return;
    }

    QMessageBox::information(this, kSuccess, QStringLiteral("\u6570\u636e\u5df2\u5bfc\u51fa\u5230\uff1a\n") + fileName);
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
        QMessageBox::warning(this, kHint, QStringLiteral("\u8bf7\u5148\u9009\u62e9\u4e00\u6761\u65e5\u5fd7\u8bb0\u5f55\u3002"));
        return;
    }

    const OperationLogEntry entry = m_currentLogs.at(currentRow);
    if (QMessageBox::question(this,
                              QStringLiteral("\u786e\u8ba4\u5220\u9664"),
                              QStringLiteral("\u786e\u5b9a\u8981\u5220\u9664\u8be5\u6761\u5386\u53f2\u8bb0\u5f55\u5417\uff1f"))
        != QMessageBox::Yes)
    {
        return;
    }

    QString errorMessage;
    if (!m_historyService.deleteOperationLog(entry.recordId, &errorMessage))
    {
        QMessageBox::critical(this, kFailed, QStringLiteral("\u5220\u9664\u5931\u8d25\uff1a") + errorMessage);
        return;
    }

    queryOperationLogs();
}
