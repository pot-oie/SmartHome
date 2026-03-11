#include "devicecontrolwidget.h"
#include "ui_devicecontrolwidget.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace
{
    const QString kHint = QStringLiteral("\u63d0\u793a");
    const QString kFailed = QStringLiteral("\u5931\u8d25");
    const QString kWarning = QStringLiteral("\u8b66\u544a");

    QString onlineStatusText(bool isOn)
    {
        return isOn ? QStringLiteral("\u25cf \u5f00\u542f") : QStringLiteral("\u25cf \u5173\u95ed");
    }
}

DeviceControlWidget::DeviceControlWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::DeviceControlWidget)
{
    ui->setupUi(this);
    reloadDevices(true);
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

void DeviceControlWidget::reloadDevices(bool reloadCategories)
{
    if (reloadCategories || m_categories.isEmpty())
    {
        m_categories = m_deviceService.categories();
    }
    m_allDevices = m_deviceService.loadDevices();
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

        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(QIcon(device.icon).pixmap(48, 48));
        iconLabel->setFixedSize(48, 48);
        cardLayout->addWidget(iconLabel);

        QVBoxLayout *infoLayout = new QVBoxLayout();
        QLabel *nameLabel = new QLabel(device.name);
        nameLabel->setStyleSheet("font-size: 14pt; font-weight: bold;");
        infoLayout->addWidget(nameLabel);

        QLabel *statusLabel = new QLabel();
        if (device.isOnline)
        {
            statusLabel->setText(onlineStatusText(device.isOn));
            statusLabel->setStyleSheet(device.isOn ? "color: #4CAF50;" : "color: #999;");
        }
        else
        {
            statusLabel->setText(QStringLiteral("\u79bb\u7ebf"));
            statusLabel->setStyleSheet("color: #f44336;");
        }
        infoLayout->addWidget(statusLabel);

        cardLayout->addLayout(infoLayout);
        cardLayout->addStretch();

        if (device.isOnline)
        {
            QPushButton *switchBtn = new QPushButton(device.isOn ? QStringLiteral("\u5173\u95ed") : QStringLiteral("\u5f00\u542f"));
            switchBtn->setFixedWidth(80);
            switchBtn->setStyleSheet(
                "QPushButton { background-color: " + QString(device.isOn ? "#4CAF50" : "#2196F3") +
                "; color: white; border: none; border-radius: 4px; padding: 8px; }"
                "QPushButton:hover { opacity: 0.8; }");

            connect(switchBtn, &QPushButton::clicked, this, [this, device]()
                    {
                const bool newState = !device.isOn;
                QString errorMessage;
                QString warningMessage;
                if (!m_deviceService.updateSwitchState(device.id, newState, &errorMessage, &warningMessage))
                {
                    QMessageBox::critical(this,
                                          kFailed,
                                          errorMessage.trimmed().isEmpty()
                                              ? QStringLiteral("\u8bbe\u5907\u72b6\u6001\u5199\u5165\u6570\u636e\u5e93\u5931\u8d25\u3002")
                                              : errorMessage);
                    return;
                }

                reloadDevices(false);
                updateDeviceListUI(ui->listCategory->currentRow());
                if (!warningMessage.trimmed().isEmpty())
                {
                    QMessageBox::warning(this, kWarning, warningMessage);
                } });
            cardLayout->addWidget(switchBtn);

            if (m_deviceService.supportsAdjust(device))
            {
                QVBoxLayout *sliderLayout = new QVBoxLayout();
                QLabel *valueLabel = new QLabel();
                valueLabel->setText(m_deviceService.valueText(device, device.value));
                sliderLayout->addWidget(valueLabel);

                QSlider *slider = new QSlider(Qt::Horizontal);
                slider->setFixedWidth(120);
                const QPair<int, int> range = m_deviceService.sliderRange(device);
                slider->setRange(range.first, range.second);
                slider->setValue(device.value);

                connect(slider, &QSlider::valueChanged, this, [this, valueLabel, device](int val)
                        { valueLabel->setText(m_deviceService.valueText(device, val)); });

                connect(slider, &QSlider::sliderReleased, this, [this, slider, device]()
                        {
                    QString errorMessage;
                    QString warningMessage;
                    if (!m_deviceService.updateDeviceValue(device, slider->value(), &errorMessage, &warningMessage))
                    {
                        QMessageBox::critical(this,
                                              kFailed,
                                              errorMessage.trimmed().isEmpty()
                                                  ? QStringLiteral("\u8bbe\u5907\u53c2\u6570\u5199\u5165\u6570\u636e\u5e93\u5931\u8d25\u3002")
                                                  : errorMessage);
                        return;
                    }

                    reloadDevices(false);
                    updateDeviceListUI(ui->listCategory->currentRow());
                    if (!warningMessage.trimmed().isEmpty())
                    {
                        QMessageBox::warning(this, kWarning, warningMessage);
                    } });

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

void DeviceControlWidget::refreshDevices()
{
    reloadDevices(false);
    updateDeviceListUI(ui->listCategory->currentRow());
}

void DeviceControlWidget::on_listCategory_currentRowChanged(int currentRow)
{
    updateDeviceListUI(currentRow);
}

void DeviceControlWidget::onDeviceSwitchToggled(bool checked)
{
    Q_UNUSED(checked);
}

void DeviceControlWidget::onDeviceSliderValueChanged(int value)
{
    Q_UNUSED(value);
}
