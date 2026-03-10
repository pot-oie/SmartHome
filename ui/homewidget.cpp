#include "homewidget.h"
#include "ui_homewidget.h"

#include <QDebug>
#include <QTimer>

HomeWidget::HomeWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
    initConnections();
    refreshDeviceStatus();

    const EnvironmentSnapshot initial = m_environmentService.generateInitialSnapshot();
    updateEnvironmentData(initial.temperature, initial.humidity);

    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]() {
        const EnvironmentSnapshot snapshot = m_environmentService.generateNextSnapshot();
        updateEnvironmentData(snapshot.temperature, snapshot.humidity);
        refreshDeviceStatus();
    });
    timer->start(5000);
}

HomeWidget::~HomeWidget()
{
    delete ui;
}

void HomeWidget::initConnections()
{
    connect(ui->btnQuickLight, &QPushButton::clicked, this, [this]() {
        emit requestQuickControl("light_living", "turn_on");
    });

    connect(ui->btnQuickAC, &QPushButton::clicked, this, [this]() {
        emit requestQuickControl("ac_living", "turn_on");
    });

    connect(ui->btnQuickCurtain, &QPushButton::clicked, this, [this]() {
        emit requestQuickControl("curtain_living", "turn_on");
    });

    connect(ui->btnGoHome, &QPushButton::clicked, this, [this]() {
        emit requestQuickControl("scene_go_home", "trigger_scene");
    });
}

void HomeWidget::onQuickControlClicked()
{
    qDebug() << "快捷控制被触发";
}

void HomeWidget::updateEnvironmentData(double temp, double hum)
{
    qDebug() << "更新环境数据: 温度 =" << temp << ", 湿度 =" << hum;

    ui->label_temperature->setText(QString::number(temp, 'f', 1) + " °C");
    ui->label_humidity->setText(QString::number(hum, 'f', 1) + " %");
    applyTemperatureColor(temp);
}

void HomeWidget::updateEnvironmentUI(const QJsonObject &data)
{
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

void HomeWidget::refreshDeviceStatus()
{
    updateDeviceStatusLabel(m_settingsService.loadDeviceStatusSummary());
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

void HomeWidget::updateDeviceStatusLabel(const DeviceStatusSummary &summary)
{
    ui->label_deviceCount->setText(QStringLiteral("在线: %1/%2").arg(summary.onlineCount).arg(summary.totalCount));
}

void HomeWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshDeviceStatus();
}
