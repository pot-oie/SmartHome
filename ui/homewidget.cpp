#include "homewidget.h"
#include "ui_homewidget.h"
#include <QDebug>

HomeWidget::HomeWidget(QWidget *parent) : QWidget(parent),
                                          ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
    initConnections();
}

HomeWidget::~HomeWidget()
{
    delete ui;
}

void HomeWidget::initConnections()
{
    // 在这里绑定额外的信号与槽
}

void HomeWidget::onQuickControlClicked()
{
    qDebug() << "快捷控制被触发";
}

void HomeWidget::updateEnvironmentData(double temp, double hum)
{
    qDebug() << "更新环境数据：温度 =" << temp << "，湿度 =" << hum;
    // 更新界面显示的温度和湿度
}

void HomeWidget::updateEnvironmentUI(const QJsonObject &data)
{
    // 解析后端数据并更新 UI 的 QLabel
}

void HomeWidget::on_btnGoHome_clicked()
{
    qDebug() << "返回首页按钮被点击";
}
