#include "alarmwidget.h"
#include "ui_alarmwidget.h"
#include <QDebug>

AlarmWidget::AlarmWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::AlarmWidget)
{
    ui->setupUi(this);
    loadThresholds();
}

AlarmWidget::~AlarmWidget()
{
    delete ui;
}

void AlarmWidget::loadThresholds() {
    // 初始化加载阈值
}

void AlarmWidget::triggerAlarm(const QJsonObject& alarmData) {
    // 触发声光报警逻辑
}

void AlarmWidget::on_btnSaveThresholds_clicked() {
    qDebug() << "保存阈值";
}

void AlarmWidget::on_btnClearLogs_clicked() {
    qDebug() << "清除报警日志";
}
