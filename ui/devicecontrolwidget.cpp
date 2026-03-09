#include "devicecontrolwidget.h"
#include "ui_devicecontrolwidget.h"
#include <QDebug>
#include <QListWidgetItem>
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
    initDeviceList();
}

DeviceControlWidget::~DeviceControlWidget()
{
    delete ui;
}

void DeviceControlWidget::initDeviceList()
{
    // 添加设备分类到左侧列表
    QStringList categories = {"全部设备", "照明设备", "空调/温控", "窗帘/遮阳", "安防设备", "影音设备"};
    ui->listCategory->clear();
    ui->listCategory->addItems(categories);
    ui->listCategory->setCurrentRow(0);

    // 模拟设备数据并显示在右侧
    updateDeviceListUI(0); // 默认显示全部设备
}

void DeviceControlWidget::updateDeviceListUI(int category)
{
    // 清空当前设备显示区
    QWidget *contentWidget = new QWidget();
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(15);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    // 假数据：不同类型的设备
    struct DeviceInfo
    {
        QString id;
        QString name;
        QString type;
        QString icon;
        bool isOnline;
        bool isOn;
        int value; // 温度/亮度/开合度等
    };

    QVector<DeviceInfo> allDevices = {
        {"light_living", "客厅主灯", "照明设备", ":/icons/light.svg", true, true, 80},
        {"light_bedroom", "卧室灯", "照明设备", ":/icons/light.svg", true, false, 60},
        {"light_kitchen", "厨房灯", "照明设备", ":/icons/light.svg", true, true, 100},

        {"ac_living", "客厅空调", "空调/温控", ":/icons/ac.svg", true, true, 24},
        {"ac_bedroom", "卧室空调", "空调/温控", ":/icons/ac.svg", true, false, 26},

        {"curtain_living", "客厅窗帘", "窗帘/遮阳", ":/icons/curtains.svg", true, true, 50},
        {"curtain_bedroom", "卧室窗帘", "窗帘/遮阳", ":/icons/curtains.svg", false, false, 0},

        {"lock_door", "前门智能锁", "安防设备", ":/icons/lock.svg", true, true, 0},
        {"camera_01", "客厅摄像头", "安防设备", ":/icons/check.svg", true, true, 0},

        {"tv_living", "客厅电视", "影音设备", ":/icons/tv.svg", true, false, 50}};

    // 根据分类过滤设备
    QVector<DeviceInfo> filteredDevices;
    QString filterType = categories[category];

    for (const auto &device : allDevices)
    {
        if (category == 0 || device.type == filterType)
        {
            filteredDevices.append(device);
        }
    }

    // 为每个设备创建控制卡片
    for (const auto &device : filteredDevices)
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

        // 右侧：控制按钮
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
                qDebug() << "切换设备：" << device.name;
                bool newState = switchBtn->text() == "开启";
                switchBtn->setText(newState ? "关闭" : "开启");
                switchBtn->setStyleSheet(
                    "QPushButton { background-color: " + QString(newState ? "#4CAF50" : "#2196F3") + 
                    "; color: white; border: none; border-radius: 4px; padding: 8px; }"
                    "QPushButton:hover { opacity: 0.8; }"
                ); });
            cardLayout->addWidget(switchBtn);

            // 对于有调节功能的设备，添加滑块
            if (device.type == "照明设备" || device.type == "空调/温控" || device.type == "窗帘/遮阳")
            {
                QVBoxLayout *sliderLayout = new QVBoxLayout();
                QLabel *valueLabel = new QLabel();
                if (device.type == "空调/温控")
                {
                    valueLabel->setText(QString::number(device.value) + "°C");
                }
                else if (device.type == "照明设备")
                {
                    valueLabel->setText("亮度: " + QString::number(device.value) + "%");
                }
                else
                {
                    valueLabel->setText("开合: " + QString::number(device.value) + "%");
                }
                sliderLayout->addWidget(valueLabel);

                QSlider *slider = new QSlider(Qt::Horizontal);
                slider->setFixedWidth(120);
                if (device.type == "空调/温控")
                {
                    slider->setRange(16, 30);
                }
                else
                {
                    slider->setRange(0, 100);
                }
                slider->setValue(device.value);
                connect(slider, &QSlider::valueChanged, this, [valueLabel, device](int val)
                        {
                    if(device.type == "空调/温控") {
                        valueLabel->setText(QString::number(val) + "°C");
                    } else if(device.type == "照明设备") {
                        valueLabel->setText("亮度: " + QString::number(val) + "%");
                    } else {
                        valueLabel->setText("开合: " + QString::number(val) + "%");
                    } });
                sliderLayout->addWidget(slider);
                cardLayout->addLayout(sliderLayout);
            }
        }

        mainLayout->addWidget(deviceCard);
    }

    mainLayout->addStretch();

    // 将内容设置到滚动区域
    if (ui->scrollArea->widget())
    {
        delete ui->scrollArea->widget();
    }
    ui->scrollArea->setWidget(contentWidget);
}

QStringList DeviceControlWidget::categories = {"全部设备", "照明设备", "空调/温控", "窗帘/遮阳", "安防设备", "影音设备"};

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
