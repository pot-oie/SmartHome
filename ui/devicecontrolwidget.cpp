#include "devicecontrolwidget.h"
#include "ui_devicecontrolwidget.h"

#include <QDebug>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QIcon>

DeviceControlWidget::DeviceControlWidget(QWidget *parent) : QWidget(parent),
                                                            ui(new Ui::DeviceControlWidget)
{
    ui->setupUi(this);
    m_categories = m_deviceService.categories();
    m_allDevices = m_deviceService.loadDefaultDevices();
    initDeviceList();
}

DeviceControlWidget::~DeviceControlWidget()
{
    delete ui;
}

void DeviceControlWidget::initDeviceList()
{
    ui->listCategory->clear();
    ui->listCategory->addItems(m_categories);
    ui->listCategory->setCurrentRow(0);

    updateDeviceListUI(0);
}

void DeviceControlWidget::updateDeviceListUI(int category)
{
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    const DeviceList filteredDevices = m_deviceService.filterDevices(m_allDevices, category, m_categories);

    for (const DeviceDefinition &device : filteredDevices)
    {
        QGroupBox *deviceCard = new QGroupBox();
        deviceCard->setTitle(device.name);
        deviceCard->setStyleSheet(
            "QGroupBox { font-weight: bold; border: 1px solid #ddd; border-radius: 8px; "
            "margin-top: 10px; padding: 15px; background-color: white; }"
            "QGroupBox::title { subcontrol-origin: margin; left: 10px; padding: 0 5px; }");

        QHBoxLayout *cardLayout = new QHBoxLayout(deviceCard);

        // 左侧：图标和状态
        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(QIcon(device.icon).pixmap(48, 48));
        iconLabel->setFixedSize(48, 48);
        cardLayout->addWidget(iconLabel);

        // 中间：设备信息
        QVBoxLayout *infoLayout = new QVBoxLayout();
        QLabel *nameLabel = new QLabel(device.name);
        nameLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
        infoLayout->addWidget(nameLabel);

        QLabel *statusLabel = new QLabel();
        if (device.isOnline)
        {
            statusLabel->setText(device.isOn ? "● 开启" : "○ 关闭");
            statusLabel->setStyleSheet(device.isOn ? "color: #4CAF50;" : "color: #999;");
        }
        else
        {
            statusLabel->setText("离线");
            statusLabel->setStyleSheet("color: #f44336;");
        }
        infoLayout->addWidget(statusLabel);

        cardLayout->addLayout(infoLayout);
        cardLayout->addStretch();

        if (device.isOnline)
        {
            QPushButton *switchBtn = new QPushButton(device.isOn ? "关闭" : "开启");
            switchBtn->setFixedWidth(80);
            switchBtn->setStyleSheet(
                "QPushButton { background-color: " + QString(device.isOn ? "#4CAF50" : "#2196F3") +
                "; color: white; border: none; border-radius: 4px; padding: 8px; }"
                "QPushButton:hover { opacity: 0.8; }");

            connect(switchBtn, &QPushButton::clicked, this, [this, device, switchBtn]()
                    {
                const bool newState = (switchBtn->text() == "开启");
                switchBtn->setText(newState ? "关闭" : "开启");
                switchBtn->setStyleSheet(
                    "QPushButton { background-color: " + QString(newState ? "#4CAF50" : "#2196F3") + 
                    "; color: white; border: none; border-radius: 4px; padding: 8px; }"
                    "QPushButton:hover { opacity: 0.8; }"
                ); 
                const QJsonObject controlCmd = m_deviceService.buildSwitchCommand(device.id, newState);
                emit requestControlDevice(controlCmd); });
            cardLayout->addWidget(switchBtn);

            if (m_deviceService.supportsAdjust(device.type))
            {
                QVBoxLayout *sliderLayout = new QVBoxLayout();
                QLabel *valueLabel = new QLabel();
                valueLabel->setText(m_deviceService.valueText(device, device.value));
                sliderLayout->addWidget(valueLabel);

                QSlider *slider = new QSlider(Qt::Horizontal);
                slider->setFixedWidth(120);
                const QPair<int, int> range = m_deviceService.sliderRange(device.type);
                slider->setRange(range.first, range.second);
                slider->setValue(device.value);

                connect(slider, &QSlider::valueChanged, this, [this, valueLabel, device](int val)
                        { valueLabel->setText(m_deviceService.valueText(device, val)); });

                connect(slider, &QSlider::sliderReleased, this, [this, slider, device]()
                        {
                    const QJsonObject controlCmd = m_deviceService.buildSetParamCommand(device, slider->value());
                    emit requestControlDevice(controlCmd); });

                sliderLayout->addWidget(slider);
                cardLayout->addLayout(sliderLayout);
            }
        }

        mainLayout->addWidget(deviceCard);
    }

    mainLayout->addStretch();

    if (ui->scrollArea->widget())
    {
        delete ui->scrollArea->widget();
    }
    ui->scrollArea->setWidget(contentWidget);
}

void DeviceControlWidget::updateDeviceStatus(const QJsonObject &statusData)
{
    // 接收后端状态更新
    qDebug() << "收到设备状态更新：" << statusData;
}

void DeviceControlWidget::on_listCategory_currentRowChanged(int currentRow)
{
    qDebug() << "切换设备分类，当前索引：" << currentRow;
    updateDeviceListUI(currentRow);
}

void DeviceControlWidget::onDeviceSwitchToggled(bool checked)
{
    qDebug() << "设备开关切换为：" << checked;
}

void DeviceControlWidget::onDeviceSliderValueChanged(int value)
{
    qDebug() << "设备参数调节为：" << value;
}