#pragma once
#include <QWidget>

namespace Ui {
class HistoryWidget;
}

class HistoryWidget : public QWidget {
    Q_OBJECT

public:
    explicit HistoryWidget(QWidget *parent = nullptr);
    ~HistoryWidget();

private slots:
    // 【UI 交互】点击查询按钮触发（根据时间范围和设备类型）
    void on_btnSearch_clicked();
    // 【UI 交互】点击导出按钮，将 TableWidget 里的数据转存为 CSV 或 Excel
    void on_btnExport_clicked();
    // 【UI 交互】切换 Tab 页（操作日志 / 环境数据折线图）
    void on_tabWidget_currentChanged(int index);

private:
    Ui::HistoryWidget *ui;
    void queryOperationLogs(); // 执行 SQL 查询日志
    void queryEnvironmentDataAndDrawChart(); // 执行 SQL 查询温湿度并绘制 QCustomPlot
};
