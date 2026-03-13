#include "historywidget.h"
#include "ui_historywidget.h"

#include "qcustomplot.h"

#include <QAbstractItemView>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHash>
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

    QString noEnvironmentDataText(bool isEnglish)
    {
        return isEnglish
                   ? QStringLiteral("No environment history data is available in the selected time range.")
                   : QStringLiteral("当前所选时间范围内没有可用的环境历史数据。");
    }

    QString loadingEnvironmentDataText(bool isEnglish)
    {
        return isEnglish
                   ? QStringLiteral("Loading environment history data...")
                   : QStringLiteral("正在加载环境历史数据...");
    }
    constexpr double kSecondsPerHour = 3600.0;
    constexpr double kSecondsPerDay = 24.0 * kSecondsPerHour;

    QSharedPointer<QCPAxisTickerDateTime> ensureDateTicker(QCustomPlot *plot)
    {
        if (!plot)
        {
            return {};
        }

        QSharedPointer<QCPAxisTickerDateTime> ticker =
            qSharedPointerDynamicCast<QCPAxisTickerDateTime>(plot->xAxis->ticker());
        if (ticker.isNull())
        {
            ticker.reset(new QCPAxisTickerDateTime);
            plot->xAxis->setTicker(ticker);
        }
        return ticker;
    }

    void updateTimeAxisStyle(QCustomPlot *plot, double lower, double upper)
    {
        const QSharedPointer<QCPAxisTickerDateTime> ticker = ensureDateTicker(plot);
        if (ticker.isNull())
        {
            return;
        }

        const double spanSeconds = qMax(upper - lower, 1.0);
        ticker->setTickCount(6);

        if (spanSeconds <= 12.0 * kSecondsPerHour)
        {
            ticker->setDateTimeFormat(QStringLiteral("hh:mm"));
        }
        else if (spanSeconds <= 3.0 * kSecondsPerDay)
        {
            ticker->setDateTimeFormat(QStringLiteral("MM-dd\nhh:mm"));
        }
        else if (spanSeconds <= 14.0 * kSecondsPerDay)
        {
            ticker->setDateTimeFormat(QStringLiteral("MM-dd hh:mm"));
        }
        else
        {
            ticker->setDateTimeFormat(QStringLiteral("MM-dd"));
        }
    }

    double chartTimePadding(double spanSeconds)
    {
        if (spanSeconds <= 0.0)
        {
            return 30.0 * 60.0;
        }

        return qBound(5.0 * 60.0, spanSeconds * 0.08, 6.0 * kSecondsPerHour);
    }
}

HistoryWidget::HistoryWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::HistoryWidget)
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

    connect(ui->dateTimeEdit_start, &QDateTimeEdit::dateChanged, this, [normalizeStartDateTime](const QDate &)
            { normalizeStartDateTime(); });
    connect(ui->dateTimeEdit_start, &QDateTimeEdit::timeChanged, this, [normalizeStartDateTime](const QTime &)
            { normalizeStartDateTime(); });
    connect(ui->dateTimeEdit_end, &QDateTimeEdit::dateChanged, this, [normalizeEndDateTime](const QDate &)
            { normalizeEndDateTime(); });
    connect(ui->dateTimeEdit_end, &QDateTimeEdit::timeChanged, this, [normalizeEndDateTime](const QTime &)
            { normalizeEndDateTime(); });

    m_btnDeleteLog = new QPushButton(QStringLiteral("删除选中日志"), this);
    if (QLayout *filterLayout = ui->groupBox_filter->layout())
    {
        filterLayout->addWidget(m_btnDeleteLog);
    }

    connect(m_btnDeleteLog, &QPushButton::clicked, this, &HistoryWidget::deleteSelectedOperationLog);
    connect(&m_historyService, &HistoryService::operationLogsReady,
            this, &HistoryWidget::onOperationLogsLoaded);
    connect(&m_historyService, &HistoryService::environmentSeriesReady,
            this, &HistoryWidget::onEnvironmentSeriesLoaded);

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
    ui->tableWidget_logs->setHorizontalHeaderLabels(
        isEnglish
            ? QStringList{QStringLiteral("Time"), QStringLiteral("User"), QStringLiteral("Operation"), QStringLiteral("Device"), QStringLiteral("Details"), QStringLiteral("Result")}
            : QStringList{QStringLiteral("时间"), QStringLiteral("用户"), QStringLiteral("操作类型"), QStringLiteral("设备"), QStringLiteral("详情"), QStringLiteral("结果")});

    if (customPlot)
    {
        customPlot->graph(0)->setName(isEnglish ? QStringLiteral("Temperature (C)") : QStringLiteral("温度 (C)"));
        customPlot->graph(1)->setName(isEnglish ? QStringLiteral("Humidity (%)") : QStringLiteral("湿度 (%)"));
        customPlot->xAxis->setLabel(isEnglish ? QStringLiteral("Time") : QStringLiteral("时间"));
        customPlot->yAxis->setLabel(isEnglish ? QStringLiteral("Environment Value") : QStringLiteral("环境数值"));
        customPlot->replot();
    }

    if (ui->label_chartPlaceholder->isVisible())
    {
        ui->label_chartPlaceholder->setText(noEnvironmentDataText(isEnglish));
    }
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
    ui->tableWidget_logs->setUpdatesEnabled(false);
    ui->tableWidget_logs->setSortingEnabled(false);
    ui->tableWidget_logs->setRowCount(0);
    ui->tableWidget_logs->setRowCount(logs.size());

    for (int i = 0; i < logs.size(); ++i)
    {
        const OperationLogEntry &entry = logs.at(i);
        QTableWidgetItem *timeItem = new QTableWidgetItem(entry.timestamp.toString(QStringLiteral("yyyy-MM-dd hh:mm:ss")));
        timeItem->setData(Qt::UserRole, entry.recordId);
        ui->tableWidget_logs->setItem(i, 0, timeItem);

        const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
        ui->tableWidget_logs->setItem(i, 1, new QTableWidgetItem(localizedText(entry.user, isEnglish)));
        ui->tableWidget_logs->setItem(i, 2, new QTableWidgetItem(localizedText(entry.operation, isEnglish)));
        ui->tableWidget_logs->setItem(i, 3, new QTableWidgetItem(localizedText(entry.device, isEnglish)));
        ui->tableWidget_logs->setItem(i, 4, new QTableWidgetItem(entry.detail));
        ui->tableWidget_logs->setItem(i, 5, new QTableWidgetItem(localizedText(entry.result, isEnglish)));
    }

    ui->tableWidget_logs->setUpdatesEnabled(true);
}

void HistoryWidget::queryOperationLogs()
{
    const QDate startDate = ui->dateTimeEdit_start->date();
    const QDate endDate = ui->dateTimeEdit_end->date();
    const QDateTime startTime(startDate, QTime(0, 0, 0));
    const QDateTime endTime(endDate.addDays(1), QTime(0, 0, 0));
    const QString deviceType = ui->comboBox_deviceType->currentText();

    ui->tableWidget_logs->setRowCount(0);
    m_historyService.asyncQueryOperationLogs(startTime, endTime, deviceType);
}

void HistoryWidget::renderEnvironmentSeries(const EnvironmentSeries &series)
{
    ui->label_chartPlaceholder->hide();
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    if (!customPlot)
    {
        customPlot = new QCustomPlot(this);
        ui->verticalLayout_charts->addWidget(customPlot);

        customPlot->addGraph();
        customPlot->graph(0)->setPen(QPen(Qt::red, 2));
        customPlot->graph(0)->setName(isEnglish ? QStringLiteral("Temperature (C)") : QStringLiteral("温度 (C)"));

        customPlot->addGraph();
        customPlot->graph(1)->setPen(QPen(Qt::blue, 2));
        customPlot->graph(1)->setName(isEnglish ? QStringLiteral("Humidity (%)") : QStringLiteral("湿度 (%)"));

        QSharedPointer<QCPAxisTickerDateTime> dateTicker(new QCPAxisTickerDateTime);
        dateTicker->setTickCount(6);
        dateTicker->setDateTimeFormat(QStringLiteral("MM-dd hh:mm"));
        customPlot->xAxis->setTicker(dateTicker);
        customPlot->xAxis->setLabel(isEnglish ? QStringLiteral("Time") : QStringLiteral("时间"));
        customPlot->yAxis->setLabel(isEnglish ? QStringLiteral("Environment Value") : QStringLiteral("环境数值"));
        customPlot->legend->setVisible(true);
    }

    QVector<double> timeData;
    QVector<double> tempData;
    QVector<double> humData;
    double minValue = 0.0;
    double maxValue = 0.0;
    bool hasValue = false;

    for (const EnvironmentPoint &point : series)
    {
        timeData.push_back(point.timestamp.toSecsSinceEpoch());
        tempData.push_back(point.temperature);
        humData.push_back(point.humidity);

        const double pointMin = qMin(point.temperature, point.humidity);
        const double pointMax = qMax(point.temperature, point.humidity);
        if (!hasValue)
        {
            minValue = pointMin;
            maxValue = pointMax;
            hasValue = true;
        }
        else
        {
            minValue = qMin(minValue, pointMin);
            maxValue = qMax(maxValue, pointMax);
        }
    }

    const QDateTime visibleStartTime(ui->dateTimeEdit_start->date(), QTime(0, 0, 0));
    const QDateTime visibleEndTime(ui->dateTimeEdit_end->date(), QTime(23, 59, 59));

    if (timeData.isEmpty())
    {
        customPlot->graph(0)->data()->clear();
        customPlot->graph(1)->data()->clear();
        updateTimeAxisStyle(customPlot,
                            visibleStartTime.toSecsSinceEpoch(),
                            visibleEndTime.toSecsSinceEpoch());
        customPlot->xAxis->setRange(visibleStartTime.toSecsSinceEpoch(), visibleEndTime.toSecsSinceEpoch());
        customPlot->yAxis->setRange(0, 100);
        customPlot->replot();
        ui->label_chartPlaceholder->setText(noEnvironmentDataText(isEnglish));
        ui->label_chartPlaceholder->show();
        return;
    }

    customPlot->graph(0)->setData(timeData, tempData);
    customPlot->graph(1)->setData(timeData, humData);

    const double dataStart = timeData.first();
    const double dataEnd = timeData.last();
    const double xPadding = chartTimePadding(dataEnd - dataStart);
    const double xLower = dataStart - xPadding;
    const double xUpper = dataEnd + xPadding;
    updateTimeAxisStyle(customPlot, xLower, xUpper);
    customPlot->xAxis->setRange(xLower, xUpper);

    double lowerBound = minValue - 5.0;
    double upperBound = maxValue + 5.0;
    if (qFuzzyCompare(lowerBound, upperBound))
    {
        lowerBound -= 1.0;
        upperBound += 1.0;
    }

    customPlot->yAxis->setRange(lowerBound, upperBound);
    customPlot->replot();
}

void HistoryWidget::queryEnvironmentDataAndDrawChart()
{
    const QDate startDate = ui->dateTimeEdit_start->date();
    const QDate endDate = ui->dateTimeEdit_end->date();
    const QDateTime startTime(startDate, QTime(0, 0, 0));
    const QDateTime endTime(endDate.addDays(1), QTime(0, 0, 0));

    ui->label_chartPlaceholder->setText(loadingEnvironmentDataText(m_languageKey == QStringLiteral("en_US")));
    ui->label_chartPlaceholder->show();
    m_historyService.asyncQueryEnvironmentSeries(startTime, endTime);
}

void HistoryWidget::onOperationLogsLoaded(OperationLogList logs)
{
    m_currentLogs = logs;
    renderOperationLogs(m_currentLogs);
    if (m_showLogLoadedMessage)
    {
        m_showLogLoadedMessage = false;
        QMessageBox::information(this, kSuccess,
                                 (m_languageKey == QStringLiteral("en_US"))
                                     ? QStringLiteral("Loaded %1 operation log entries.").arg(m_currentLogs.size())
                                     : QStringLiteral("已加载 %1 条操作日志记录。").arg(m_currentLogs.size()));
    }
}

void HistoryWidget::onEnvironmentSeriesLoaded(EnvironmentSeries series)
{
    renderEnvironmentSeries(series);
}

void HistoryWidget::on_btnSearch_clicked()
{
    if (ui->dateTimeEdit_start->date() > ui->dateTimeEdit_end->date())
    {
        QMessageBox::warning(this, kFailed,
                             (m_languageKey == QStringLiteral("en_US"))
                                 ? QStringLiteral("The start time cannot be later than the end time.")
                                 : QStringLiteral("开始时间不能晚于结束时间。"));
        return;
    }

    m_showLogLoadedMessage = true;
    queryOperationLogs();
    if (ui->tabWidget->currentIndex() == 1)
    {
        queryEnvironmentDataAndDrawChart();
    }
}

void HistoryWidget::on_btnExport_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(
        this,
        (m_languageKey == QStringLiteral("en_US")) ? QStringLiteral("Export Data") : QStringLiteral("导出数据"),
        QDir::homePath() + QStringLiteral("/operation_logs.xlsx"),
        QStringLiteral("Excel Files (*.xlsx)"));
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
        QMessageBox::warning(this, kFailed,
                             (m_languageKey == QStringLiteral("en_US"))
                                 ? QStringLiteral("There is no log data to export.")
                                 : QStringLiteral("当前没有可导出的日志数据。"));
        return;
    }

    QString errorMessage;
    if (!m_historyService.exportOperationLogsToExcel(fileName, m_currentLogs, &errorMessage))
    {
        QMessageBox::critical(this, kFailed,
                              ((m_languageKey == QStringLiteral("en_US"))
                                   ? QStringLiteral("Export failed: ")
                                   : QStringLiteral("导出失败：")) + errorMessage);
        return;
    }

    QMessageBox::information(this, kSuccess,
                             ((m_languageKey == QStringLiteral("en_US"))
                                  ? QStringLiteral("Data exported to:\n")
                                  : QStringLiteral("数据已导出到：\n")) + fileName);
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
        QMessageBox::warning(this, kHint,
                             (m_languageKey == QStringLiteral("en_US"))
                                 ? QStringLiteral("Please select a log entry first.")
                                 : QStringLiteral("请先选择一条日志记录。"));
        return;
    }

    const OperationLogEntry entry = m_currentLogs.at(currentRow);
    if (QMessageBox::question(this,
                              (m_languageKey == QStringLiteral("en_US")) ? QStringLiteral("Confirm Delete") : QStringLiteral("确认删除"),
                              (m_languageKey == QStringLiteral("en_US"))
                                  ? QStringLiteral("Are you sure you want to delete this history entry?")
                                  : QStringLiteral("确定要删除该条历史记录吗？")) != QMessageBox::Yes)
    {
        return;
    }

    QString errorMessage;
    if (!m_historyService.deleteOperationLog(entry.recordId, &errorMessage))
    {
        QMessageBox::critical(this, kFailed,
                              ((m_languageKey == QStringLiteral("en_US"))
                                   ? QStringLiteral("Delete failed: ")
                                   : QStringLiteral("删除失败：")) + errorMessage);
        return;
    }

    queryOperationLogs();
}
