#include "homewidget.h"
#include "ui_homewidget.h"
#include <QDebug>
#include <QTimer>

HomeWidget::HomeWidget(QWidget *parent) : QWidget(parent),
                                          ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
    initConnections();

    // 初始化假数据
    updateEnvironmentData(26.5, 45.0);

    // 使用定时器模拟数据更新（每5秒更新一次）
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]()
            {
        // 模拟环境数据波动
        double temp = 20.0 + (rand() % 120) / 10.0; // 20-32°C
        double hum = 30.0 + (rand() % 400) / 10.0;  // 30-70%
        
        ui->label_temperature->setText(QString::number(temp, 'f', 1) + " °C");
        ui->label_humidity->setText(QString::number(hum, 'f', 1) + " %");
        
        // 根据数值设置颜色指示
        if(temp > 28) {
            ui->label_temperature->setStyleSheet("color: #f44336; font-weight: bold;");
        } else if(temp < 22) {
            ui->label_temperature->setStyleSheet("color: #2196F3; font-weight: bold;");
        } else {
            ui->label_temperature->setStyleSheet("color: #4CAF50; font-weight: bold;");
        } });
    timer->start(5000);
}

HomeWidget::~HomeWidget()
{
    delete ui;
}

void HomeWidget::initConnections()
{
    // 【优化】将界面上的快捷按钮与发射指令的信号连接起来
    connect(ui->btnQuickLight, &QPushButton::clicked, this, [this]()
            { emit requestQuickControl("light_living", "turn_on"); });

    connect(ui->btnQuickAC, &QPushButton::clicked, this, [this]()
            { emit requestQuickControl("ac_living", "turn_on"); });

    connect(ui->btnQuickCurtain, &QPushButton::clicked, this, [this]()
            { emit requestQuickControl("curtain_living", "turn_on"); });

    // 点击“回家模式”按钮时，交由后端或 MainWindow 路由到场景模块
    connect(ui->btnGoHome, &QPushButton::clicked, this, [this]()
            { emit requestQuickControl("scene_go_home", "trigger_scene"); });
}

void HomeWidget::onQuickControlClicked()
{
    qDebug() << "快捷控制被触发";
}

void HomeWidget::updateEnvironmentData(double temp, double hum)
{
    qDebug() << "更新环境数据：温度 =" << temp << "，湿度 =" << hum;
    // 更新界面显示的温度和湿度
    ui->label_temperature->setText(QString::number(temp, 'f', 1) + " °C");
    ui->label_humidity->setText(QString::number(hum, 'f', 1) + " %");
}

void HomeWidget::updateEnvironmentUI(const QJsonObject &data)
{
    // 解析后端数据并更新 UI 的 QLabel
    if (data.contains("temperature"))
    {
        ui->label_temperature->setText(QString::number(data["temperature"].toDouble(), 'f', 1) + " °C");
    }
    if (data.contains("humidity"))
    {
        ui->label_humidity->setText(QString::number(data["humidity"].toDouble(), 'f', 1) + " %");
    }
}

void HomeWidget::on_btnGoHome_clicked()
{
    qDebug() << "返回首页按钮被点击";
}