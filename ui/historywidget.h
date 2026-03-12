#pragma once

#include <QWidget>
#include <QShowEvent>

#include "services/deviceservice.h"
#include "services/historyservice.h"

namespace Ui
{
    class HistoryWidget;
}

class QCustomPlot;
class QPushButton;

class HistoryWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HistoryWidget(QWidget *parent = nullptr);
    ~HistoryWidget();
    void applyLanguage(const QString &languageKey);

protected:
    void showEvent(QShowEvent *event) override;

private slots:
    void on_btnSearch_clicked();
    void on_btnExport_clicked();
    void on_tabWidget_currentChanged(int index);
    void deleteSelectedOperationLog();

private:
    void queryOperationLogs();
    void queryEnvironmentDataAndDrawChart();

private:
    Ui::HistoryWidget *ui;
    HistoryService m_historyService;
    DeviceService m_deviceService;
    OperationLogList m_currentLogs;
    QCustomPlot *customPlot = nullptr;
    QPushButton *m_btnDeleteLog = nullptr;
    QString m_languageKey = QStringLiteral("zh_CN");
};
