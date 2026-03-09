#include "historywidget.h"
#include "ui_historywidget.h"
#include <QDebug>

HistoryWidget::HistoryWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::HistoryWidget)
{
    ui->setupUi(this);
}

HistoryWidget::~HistoryWidget()
{
    delete ui;
}

void HistoryWidget::queryOperationLogs() {
    // 查询日志逻辑
}

void HistoryWidget::queryEnvironmentDataAndDrawChart() {
    // 查询数据绘图逻辑
}

void HistoryWidget::on_btnSearch_clicked() {
    qDebug() << "执行查询";
    queryOperationLogs();
}

void HistoryWidget::on_btnExport_clicked() {
    qDebug() << "执行导出";
}

void HistoryWidget::on_tabWidget_currentChanged(int index) {
    qDebug() << "Tab页切换：" << index;
}
