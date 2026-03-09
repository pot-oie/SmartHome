#include "homewidget.h"
#include "ui_homewidget.h"
#include <QDebug>
#include <QTimer>

HomeWidget::HomeWidget(QWidget *parent) : QWidget(parent),
                                          ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
    initConnections();

    // 初始环境数据由 service 提供
    const EnvironmentSnapshot initial = m_environmentService.generateInitialSnapshot();
    updateEnvironmentData(initial.temperature, initial.humidity);

    // 使用定时器模拟数据更新（每5秒更新一次）
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]()
            {
                const EnvironmentSnapshot snapshot = m_environmentService.generateNextSnapshot();
                updateEnvironmentData(snapshot.temperature, snapshot.humidity); });
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

    ui->label_temperature->setText(QString::number(temp, 'f', 1) + " °C");
    ui->label_humidity->setText(QString::number(hum, 'f', 1) + " %");
    applyTemperatureColor(temp);
}

void HomeWidget::updateEnvironmentUI(const QJsonObject &data)
{
    // 解析后端数据并更新 UI 的 QLabel
    if (data.contains("temperature"))
    {
        const double temperature = data["temperature"].toDouble();
        ui->label_temperature->setText(QString::number(temperature, 'f', 1) + " °C");
        applyTemperatureColor(temperature);
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

void HomeWidget::applyTemperatureColor(double temperature)
{
    const QString color = m_environmentService.temperatureColor(temperature);
    ui->label_temperature->setStyleSheet(QString("color: %1; font-weight: bold;").arg(color));
}