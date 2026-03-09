#include "devicecontrolwidget.h"
#include "ui_devicecontrolwidget.h"
#include <QDebug>

DeviceControlWidget::DeviceControlWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::DeviceControlWidget)
{
    ui->setupUi(this);
    initDeviceList();
}

DeviceControlWidget::~DeviceControlWidget()
{
    delete ui;
}

void DeviceControlWidget::initDeviceList() {
    // 初始化列表数据
}

void DeviceControlWidget::updateDeviceStatus(const QJsonObject& statusData) {
    // 接收后端状态更新
}

void DeviceControlWidget::on_listCategory_currentRowChanged(int currentRow) {
    qDebug() << "切换设备分类，当前索引：" << currentRow;
}

void DeviceControlWidget::onDeviceSwitchToggled(bool checked) {
    qDebug() << "设备开关切换为：" << checked;
}

void DeviceControlWidget::onDeviceSliderValueChanged(int value) {
    qDebug() << "设备参数调节为：" << value;
}
