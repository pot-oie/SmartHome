#include "devicecontrolwidget.h"
#include "ui_devicecontrolwidget.h"

#include <QApplication>
#include <QColor>
#include <QMetaObject>
#include <QHBoxLayout>
#include <QIcon>
#include <QHash>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QEvent>
#include <QPalette>
#include <QSlider>
#include <QVBoxLayout>
#include <QComboBox>
#include <QSpinBox>

namespace
{
    const QString kFailed = QStringLiteral("\u5931\u8d25");
    const QString kWarning = QStringLiteral("\u8b66\u544a");

    QColor blendColor(const QColor &base, const QColor &overlay, qreal ratio)
    {
        const qreal clampedRatio = qBound<qreal>(0.0, ratio, 1.0);
        return QColor::fromRgbF(base.redF() * (1.0 - clampedRatio) + overlay.redF() * clampedRatio,
                                base.greenF() * (1.0 - clampedRatio) + overlay.greenF() * clampedRatio,
                                base.blueF() * (1.0 - clampedRatio) + overlay.blueF() * clampedRatio,
                                1.0);
    }

    QString colorCss(const QColor &color)
    {
        return color.name(QColor::HexRgb);
    }

    bool isDarkTheme(const QPalette &palette)
    {
        return palette.color(QPalette::Window).lightness() < 128;
    }

    QString transparentWidgetStyle()
    {
        return QStringLiteral("background: transparent; border: none;");
    }

    QString cardStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor cardBackground = dark ? blendColor(window, QColor("#FFFFFF"), 0.08) : QColor("#FFFFFF");
        const QColor borderColor = dark ? blendColor(window, QColor("#FFFFFF"), 0.12) : blendColor(window, QColor("#000000"), 0.10);
        return QStringLiteral("background: %1; border: 1px solid %2; border-radius: 12px;")
            .arg(colorCss(cardBackground), colorCss(borderColor));
    }

    QString titleTextStyle(const QPalette &palette)
    {
        return QStringLiteral("font-size: 11pt; font-weight: 700; color: %1;")
            .arg(colorCss(palette.color(QPalette::WindowText)));
    }

    QString mutedTextStyle(const QPalette &palette, int pointSize = 10, int weight = 400)
    {
        const bool dark = isDarkTheme(palette);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor window = palette.color(QPalette::Window);
        const QColor muted = blendColor(text, window, dark ? 0.38 : 0.58);
        return QStringLiteral("font-size: %1pt; font-weight: %2; color: %3;")
            .arg(pointSize)
            .arg(weight)
            .arg(colorCss(muted));
    }

    QString metricTextStyle(const QPalette &palette)
    {
        return QStringLiteral("font-size: 10pt; font-weight: 600; color: %1;")
            .arg(colorCss(palette.color(QPalette::WindowText)));
    }

    QString panelStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor panelBackground = dark ? blendColor(window, QColor("#FFFFFF"), 0.11) : blendColor(window, QColor("#000000"), 0.02);
        const QColor borderColor = dark ? blendColor(window, QColor("#FFFFFF"), 0.20) : blendColor(window, QColor("#000000"), 0.09);
        return QStringLiteral("background: %1; border: 1px solid %2; border-radius: 10px;")
            .arg(colorCss(panelBackground), colorCss(borderColor));
    }

    bool isSwitchOnlySecurityDevice(const DeviceDefinition &device)
    {
        return device.id == QStringLiteral("lock_door")
               || device.id == QStringLiteral("camera_01")
               || device.name == QStringLiteral("前门智能锁")
               || device.name == QStringLiteral("客厅摄像头");
    }

    bool isVolumeOnlyTvDevice(const DeviceDefinition &device)
    {
        return device.id == QStringLiteral("tv_living")
               || device.name == QStringLiteral("客厅电视");
    }

    bool isLimitedCurtainDevice(const DeviceDefinition &device)
    {
        return device.id == QStringLiteral("curtain_bedroom")
               || device.name == QStringLiteral("卧室窗帘");
    }

    bool shouldHideExtraSettings(const DeviceDefinition &device)
    {
        return isSwitchOnlySecurityDevice(device)
               || isVolumeOnlyTvDevice(device)
               || isLimitedCurtainDevice(device);
    }

    bool isSensorDevice(const DeviceDefinition &device)
    {
        return device.type.contains(QStringLiteral("传感"))
               || device.name.contains(QStringLiteral("传感器"));
    }

    QString statusTextForDevice(const DeviceDefinition &device, bool isEnglish)
    {
        if (!device.isOnline)
        {
            return isEnglish ? QStringLiteral("Offline") : QStringLiteral("离线");
        }
        if (isSensorDevice(device))
        {
            return isEnglish ? QStringLiteral("Monitoring") : QStringLiteral("在线监测中");
        }
        return device.isOn
                   ? (isEnglish ? QStringLiteral("Online · On") : QStringLiteral("在线 · 已开启"))
                   : (isEnglish ? QStringLiteral("Online · Off") : QStringLiteral("在线 · 已关闭"));
    }

    QString statusStyleForDevice(const DeviceDefinition &device, const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        QColor textColor;
        QColor backgroundColor;

        if (!device.isOnline)
        {
            textColor = dark ? QColor("#FF9AA9") : QColor("#B3261E");
            backgroundColor = dark ? blendColor(window, QColor("#FF9AA9"), 0.14) : QColor("#FDECEE");
        }
        else if (isSensorDevice(device))
        {
            textColor = dark ? QColor("#71E6D8") : QColor("#0F766E");
            backgroundColor = dark ? blendColor(window, QColor("#71E6D8"), 0.14) : QColor("#E6FFFA");
        }
        else if (device.isOn)
        {
            textColor = dark ? QColor("#8AD79A") : QColor("#137333");
            backgroundColor = dark ? blendColor(window, QColor("#8AD79A"), 0.14) : QColor("#E9F7EF");
        }

        if (!textColor.isValid())
        {
            textColor = blendColor(palette.color(QPalette::WindowText), window, dark ? 0.20 : 0.45);
            backgroundColor = dark ? blendColor(window, QColor("#FFFFFF"), 0.08) : QColor("#F1F3F4");
        }

        return QStringLiteral("color: %1; background: %2; border-radius: 10px; padding: 4px 10px; font-size: 9pt; font-weight: 600;")
            .arg(colorCss(textColor), colorCss(backgroundColor));
    }

    QString actionTextForDevice(const DeviceDefinition &device, bool isEnglish)
    {
        return device.isOn
                   ? (isEnglish ? QStringLiteral("Turn Off") : QStringLiteral("关闭设备"))
                   : (isEnglish ? QStringLiteral("Turn On") : QStringLiteral("开启设备"));
    }

    QString actionStyleForDevice(const DeviceDefinition &device, const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor accent = palette.color(QPalette::Highlight);
        const QColor window = palette.color(QPalette::Window);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor ghostBorder = dark ? blendColor(window, QColor("#FFFFFF"), 0.20) : blendColor(window, QColor("#000000"), 0.12);
        const QColor ghostBg = dark ? blendColor(window, QColor("#FFFFFF"), 0.04) : QColor("#FFFFFF");
        const QColor ghostHover = dark ? blendColor(ghostBg, QColor("#FFFFFF"), 0.08) : blendColor(ghostBg, QColor("#000000"), 0.04);
        const QColor ghostPressed = dark ? blendColor(ghostBg, QColor("#FFFFFF"), 0.12) : blendColor(ghostBg, QColor("#000000"), 0.07);

        if (device.isOn)
        {
            return QStringLiteral("QPushButton { background: %1; color: %2; border: 1px solid %3; border-radius: 16px; padding: 6px 14px; font-size: 9pt; font-weight: 600; } QPushButton:hover { background: %4; } QPushButton:pressed { background: %5; }")
            .arg(colorCss(ghostBg), colorCss(text), colorCss(ghostBorder), colorCss(ghostHover), colorCss(ghostPressed));
        }

        const QColor accentText = dark ? blendColor(accent, QColor("#FFFFFF"), 0.22) : blendColor(accent, QColor("#000000"), 0.05);
        const QColor offBorder = dark ? blendColor(accent, window, 0.58) : blendColor(accent, window, 0.78);
        const QColor offBg = dark ? blendColor(accent, window, 0.86) : blendColor(accent, window, 0.92);
        const QColor hover = dark ? blendColor(offBg, QColor("#FFFFFF"), 0.05) : blendColor(offBg, QColor("#000000"), 0.03);
        const QColor pressed = dark ? blendColor(offBg, QColor("#FFFFFF"), 0.10) : blendColor(offBg, QColor("#000000"), 0.06);
        return QStringLiteral("QPushButton { background: %1; color: %2; border: none; border-radius: 16px; padding: 6px 14px; font-size: 9pt; font-weight: 600; } QPushButton:hover { background: %3; } QPushButton:pressed { background: %4; }")
            .arg(colorCss(offBg), colorCss(accentText), colorCss(hover), colorCss(pressed))
            .replace(QStringLiteral("border: none;"), QStringLiteral("border: 1px solid %1;").arg(colorCss(offBorder)));
    }

    QString sliderStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor groove = dark ? blendColor(window, QColor("#FFFFFF"), 0.18) : blendColor(window, QColor("#000000"), 0.14);
        const QColor accent = palette.color(QPalette::Highlight);
        const QColor handle = dark ? blendColor(window, QColor("#FFFFFF"), 0.10) : QColor("#FFFFFF");
        return QStringLiteral("QSlider::groove:horizontal { height: 4px; background: %1; border-radius: 2px; } "
                              "QSlider::sub-page:horizontal { background: %2; border-radius: 2px; } "
                              "QSlider::add-page:horizontal { background: %1; border-radius: 2px; } "
                              "QSlider::handle:horizontal { width: 14px; margin: -5px 0; border-radius: 7px; background: %3; border: 2px solid %2; }")
            .arg(colorCss(groove), colorCss(accent), colorCss(handle));
    }

    QString parameterLabelForDevice(const DeviceDefinition &device, bool isEnglish)
    {
        if (device.type.contains(QStringLiteral("空调")))
        {
            return isEnglish ? QStringLiteral("Temperature") : QStringLiteral("温度");
        }
        if (device.type.contains(QStringLiteral("照明")))
        {
            return isEnglish ? QStringLiteral("Brightness") : QStringLiteral("亮度");
        }
        if (device.type.contains(QStringLiteral("窗帘")))
        {
            return isEnglish ? QStringLiteral("Open Level") : QStringLiteral("开合度");
        }
        if (device.type.contains(QStringLiteral("影音")) || device.name.contains(QStringLiteral("电视")))
        {
            return isEnglish ? QStringLiteral("Volume") : QStringLiteral("音量");
        }
        return isEnglish ? QStringLiteral("Value") : QStringLiteral("当前值");
    }

    QString localizedValueText(const DeviceDefinition &device, int value, bool isEnglish)
    {
        if (device.type.contains(QStringLiteral("空调")))
        {
            return isEnglish ? QStringLiteral("%1 °C").arg(value) : QStringLiteral("%1度").arg(value);
        }
        if (device.type.contains(QStringLiteral("照明"))
            || device.type.contains(QStringLiteral("窗帘"))
            || device.type.contains(QStringLiteral("影音"))
            || device.name.contains(QStringLiteral("电视")))
        {
            return QStringLiteral("%1%").arg(value);
        }
        if (!device.valueUnit.trimmed().isEmpty())
        {
            return QString::number(value) + device.valueUnit;
        }
        return QString::number(value);
    }

    QString localizedCategory(const QString &category, bool isEnglish)
    {
        if (!isEnglish)
        {
            return category;
        }

        static const QHash<QString, QString> map = {
            {QStringLiteral("全部设备"), QStringLiteral("All Devices")},
            {QStringLiteral("照明设备"), QStringLiteral("Lighting")},
            {QStringLiteral("空调设备"), QStringLiteral("Climate")},
            {QStringLiteral("窗帘设备"), QStringLiteral("Curtains")},
            {QStringLiteral("安防设备"), QStringLiteral("Security")},
            {QStringLiteral("影音设备"), QStringLiteral("Media")},
            {QStringLiteral("传感设备"), QStringLiteral("Sensors")}};
        return map.value(category, category);
    }
}

DeviceControlWidget::DeviceControlWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::DeviceControlWidget)
{
    ui->setupUi(this);
    applyLanguage(QStringLiteral("zh_CN"));
    reloadDevices(true);
    initDeviceList();
}

DeviceControlWidget::~DeviceControlWidget()
{
    delete ui;
}

void DeviceControlWidget::applyLanguage(const QString &languageKey)
{
    if (languageKey.trimmed().isEmpty())
    {
        return;
    }

    m_languageKey = languageKey;
    ui->groupBox_category->setTitle(m_languageKey == QStringLiteral("en_US")
                                        ? QStringLiteral("Device Categories")
                                        : QStringLiteral("设备分类"));
    ui->groupBox_devices->setTitle(m_languageKey == QStringLiteral("en_US")
                                       ? QStringLiteral("Device Controls")
                                       : QStringLiteral("设备控制面板"));

    const int currentRow = qMax(0, ui->listCategory->currentRow());
    ui->listCategory->clear();
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    QStringList displayCategories;
    for (const QString &category : m_categories)
    {
        displayCategories.push_back(localizedCategory(category, isEnglish));
    }
    ui->listCategory->addItems(displayCategories);
    ui->listCategory->setCurrentRow(qMin(currentRow, ui->listCategory->count() - 1));
    updateDeviceListUI(ui->listCategory->currentRow());
}

void DeviceControlWidget::initDeviceList()
{
    ui->listCategory->clear();
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    QStringList displayCategories;
    for (const QString &category : m_categories)
    {
        displayCategories.push_back(localizedCategory(category, isEnglish));
    }
    ui->listCategory->addItems(displayCategories);
    ui->listCategory->setCurrentRow(0);
    updateDeviceListUI(0);
}

void DeviceControlWidget::reloadDevices(bool reloadCategories)
{
    if (reloadCategories || m_categories.isEmpty())
    {
        m_categories = m_deviceService.categories();
    }
    m_allDevices = m_deviceService.loadDefaultDevices();
}

void DeviceControlWidget::updateDeviceListUI(int category)
{
    QWidget *contentWidget = new QWidget();
    contentWidget->setAttribute(Qt::WA_StyledBackground, true);
    contentWidget->setStyleSheet(transparentWidgetStyle());
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    const DeviceList filteredDevices = m_deviceService.filterDevices(m_allDevices, category, m_categories);
    const QPalette palette = this->palette();
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));

    auto refreshCurrentCategory = [this]() {
        reloadDevices(false);
        updateDeviceListUI(ui->listCategory->currentRow());
    };

    auto createSwitchButton = [this, &refreshCurrentCategory, palette, isEnglish](const DeviceDefinition &device) {
        QPushButton *switchBtn = new QPushButton(actionTextForDevice(device, isEnglish));
        switchBtn->setCursor(Qt::PointingHandCursor);
        switchBtn->setFixedWidth(116);
        switchBtn->setMinimumHeight(34);
        switchBtn->setStyleSheet(actionStyleForDevice(device, palette));
        connect(switchBtn, &QPushButton::clicked, this, [this, device, refreshCurrentCategory]() {
            const bool newState = !device.isOn;
            QString errorMessage;
            QString warningMessage;
            if (!m_deviceService.updateSwitchState(device.id, newState, &errorMessage, &warningMessage))
            {
                QMessageBox::critical(this, kFailed, errorMessage.trimmed().isEmpty() ? QStringLiteral("设备状态写入数据库失败。") : errorMessage);
                return;
            }

            const QJsonObject controlCmd = m_deviceService.buildSwitchCommand(device.id, newState);
            emit requestControlDevice(controlCmd);
            refreshCurrentCategory();
            if (!warningMessage.trimmed().isEmpty())
            {
                QMessageBox::warning(this, kWarning, warningMessage);
            }
        });
        return switchBtn;
    };

    auto addSliderControl = [this, &refreshCurrentCategory, palette](QVBoxLayout *targetLayout,
                                                                     const DeviceDefinition &device,
                                                                     const QString &labelText,
                                                                     int minValue,
                                                                     int maxValue,
                                                                     const auto &valueFormatter) {
        QWidget *section = new QWidget();
        section->setAttribute(Qt::WA_StyledBackground, true);
        section->setStyleSheet(transparentWidgetStyle());
        QVBoxLayout *sectionLayout = new QVBoxLayout(section);
        sectionLayout->setContentsMargins(0, 0, 0, 0);
        sectionLayout->setSpacing(6);

        QHBoxLayout *titleRow = new QHBoxLayout();
        titleRow->setContentsMargins(0, 0, 0, 0);
        titleRow->setSpacing(8);

        QLabel *titleLabel = new QLabel(labelText);
        titleLabel->setStyleSheet(metricTextStyle(palette));
        titleLabel->setMinimumWidth(96);
        titleRow->addWidget(titleLabel);

        QLabel *valueLabel = new QLabel(valueFormatter(device.value));
        valueLabel->setStyleSheet(mutedTextStyle(palette, 10, 500));
        valueLabel->setMinimumWidth(80);
        valueLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        titleRow->addWidget(valueLabel);
        titleRow->addStretch();
        sectionLayout->addLayout(titleRow);

        QSlider *slider = new QSlider(Qt::Horizontal);
        slider->setRange(minValue, maxValue);
        slider->setValue(device.value);
        slider->setStyleSheet(sliderStyle(palette));
        connect(slider, &QSlider::valueChanged, this, [valueLabel, valueFormatter](int value) {
            valueLabel->setText(valueFormatter(value));
        });
        connect(slider, &QSlider::sliderReleased, this, [this, slider, device, refreshCurrentCategory]() {
            QString errorMessage;
            QString warningMessage;
            if (!m_deviceService.updateDeviceValue(device, slider->value(), &errorMessage, &warningMessage))
            {
                QMessageBox::critical(this, kFailed, errorMessage.trimmed().isEmpty() ? QStringLiteral("设备参数写入数据库失败。") : errorMessage);
                return;
            }

            const QJsonObject controlCmd = m_deviceService.buildSetParamCommand(device, slider->value());
            emit requestControlDevice(controlCmd);
            refreshCurrentCategory();
            if (!warningMessage.trimmed().isEmpty())
            {
                QMessageBox::warning(this, kWarning, warningMessage);
            }
        });
        sectionLayout->addWidget(slider);

        targetLayout->addWidget(section);
    };

    for (const DeviceDefinition &device : filteredDevices)
    {
        QWidget *deviceCard = new QWidget();
        deviceCard->setAttribute(Qt::WA_StyledBackground, true);
        deviceCard->setStyleSheet(cardStyle(palette));

        QVBoxLayout *cardLayout = new QVBoxLayout(deviceCard);
        cardLayout->setSpacing(10);
        cardLayout->setContentsMargins(16, 14, 16, 14);

        QHBoxLayout *headerLayout = new QHBoxLayout();
        headerLayout->setSpacing(12);
        headerLayout->setContentsMargins(0, 0, 0, 0);

        QLabel *iconLabel = new QLabel();
        iconLabel->setPixmap(QIcon(device.icon).pixmap(28, 28));
        iconLabel->setFixedSize(32, 32);
        headerLayout->addWidget(iconLabel, 0, Qt::AlignTop);

        QWidget *infoWidget = new QWidget();
        infoWidget->setAttribute(Qt::WA_StyledBackground, true);
        infoWidget->setStyleSheet(transparentWidgetStyle());
        QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
        infoLayout->setContentsMargins(0, 0, 0, 0);
        infoLayout->setSpacing(6);

        QLabel *nameLabel = new QLabel(device.name);
        nameLabel->setStyleSheet(titleTextStyle(palette));
        infoLayout->addWidget(nameLabel);

        QHBoxLayout *metaLayout = new QHBoxLayout();
        metaLayout->setContentsMargins(0, 0, 0, 0);
        metaLayout->setSpacing(6);

        QLabel *statusLabel = new QLabel(statusTextForDevice(device, isEnglish));
        statusLabel->setText(statusTextForDevice(device, isEnglish));
        statusLabel->setStyleSheet(statusStyleForDevice(device, palette));
        metaLayout->addWidget(statusLabel, 0, Qt::AlignLeft);
        metaLayout->addStretch();

        infoLayout->addLayout(metaLayout);
        headerLayout->addWidget(infoWidget, 1);

        const bool isSwitchOnlyDevice = isSwitchOnlySecurityDevice(device);
        const bool isVolumeOnlyDevice = isVolumeOnlyTvDevice(device);
        const bool isLimitedCurtain = isLimitedCurtainDevice(device);
        const bool isReadOnlySensor = isSensorDevice(device);
        const bool supportsAdjust = m_deviceService.supportsAdjust(device);

        if (device.isOnline && !isVolumeOnlyDevice && !isReadOnlySensor)
        {
            headerLayout->addWidget(createSwitchButton(device), 0, Qt::AlignTop);
        }

        cardLayout->addLayout(headerLayout);

        if (!device.isOnline)
        {
            QLabel *offlineHintLabel = new QLabel(isEnglish
                                                      ? QStringLiteral("This device is offline and unavailable.")
                                                      : QStringLiteral("设备当前离线，暂不可执行控制操作。"));
            offlineHintLabel->setStyleSheet(mutedTextStyle(palette));
            cardLayout->addWidget(offlineHintLabel);
            mainLayout->addWidget(deviceCard);
            continue;
        }

        if (isReadOnlySensor)
        {
            QLabel *readingLabel = new QLabel((isEnglish ? QStringLiteral("Current reading: ") : QStringLiteral("当前读数："))
                                              + localizedValueText(device, device.value, isEnglish));
            readingLabel->setStyleSheet(metricTextStyle(palette));
            cardLayout->addWidget(readingLabel);
            mainLayout->addWidget(deviceCard);
            continue;
        }

        if (isSwitchOnlyDevice)
        {
            mainLayout->addWidget(deviceCard);
            continue;
        }

        QVBoxLayout *bodyLayout = new QVBoxLayout();
        bodyLayout->setContentsMargins(40, 0, 0, 0);
        bodyLayout->setSpacing(8);

        if (isVolumeOnlyDevice)
        {
            addSliderControl(bodyLayout, device, isEnglish ? QStringLiteral("Volume") : QStringLiteral("音量"), 0, 100, [](int value) {
                return QString::number(value) + QStringLiteral("%");
            });
        }
        else if (isLimitedCurtain)
        {
            addSliderControl(bodyLayout, device, isEnglish ? QStringLiteral("Open Level") : QStringLiteral("开合度"), 0, 100, [](int value) {
                return QString::number(value) + QStringLiteral("%");
            });
        }
        else
        {
            if (supportsAdjust)
            {
                const QPair<int, int> range = m_deviceService.sliderRange(device);
                addSliderControl(bodyLayout,
                                 device,
                                 parameterLabelForDevice(device, isEnglish),
                                 range.first,
                                 range.second,
                                 [device, isEnglish](int value) {
                                     return localizedValueText(device, value, isEnglish);
                                 });
            }

            if (!shouldHideExtraSettings(device) && device.type.contains(QStringLiteral("空调")))
            {
                QWidget *extraPanel = new QWidget();
                extraPanel->setAttribute(Qt::WA_StyledBackground, true);
                extraPanel->setStyleSheet(panelStyle(palette));
                QHBoxLayout *extraLayout = new QHBoxLayout(extraPanel);
                extraLayout->setContentsMargins(12, 8, 12, 8);
                extraLayout->setSpacing(10);

                QLabel *modeLabel = new QLabel(QStringLiteral("模式"));
                modeLabel->setText(isEnglish ? QStringLiteral("Mode") : QStringLiteral("模式"));
                modeLabel->setStyleSheet(mutedTextStyle(palette));
                modeLabel->setMinimumWidth(44);
                QComboBox *modeBox = new QComboBox();
                modeBox->setMinimumWidth(84);
                modeBox->addItems(isEnglish
                                      ? QStringList{QStringLiteral("Cool"), QStringLiteral("Heat"), QStringLiteral("Dry"), QStringLiteral("Fan")}
                                      : QStringList{QStringLiteral("制冷"), QStringLiteral("制热"), QStringLiteral("除湿"), QStringLiteral("送风")});

                QLabel *fanLabel = new QLabel(QStringLiteral("风速"));
                fanLabel->setText(isEnglish ? QStringLiteral("Fan") : QStringLiteral("风速"));
                fanLabel->setStyleSheet(mutedTextStyle(palette));
                fanLabel->setMinimumWidth(36);
                QComboBox *fanBox = new QComboBox();
                fanBox->setMinimumWidth(72);
                fanBox->addItems(isEnglish
                                     ? QStringList{QStringLiteral("Low"), QStringLiteral("Medium"), QStringLiteral("High")}
                                     : QStringList{QStringLiteral("低"), QStringLiteral("中"), QStringLiteral("高")});

                QLabel *timerLabel = new QLabel(QStringLiteral("定时"));
                timerLabel->setText(isEnglish ? QStringLiteral("Timer") : QStringLiteral("定时"));
                timerLabel->setStyleSheet(mutedTextStyle(palette));
                timerLabel->setMinimumWidth(44);
                QSpinBox *timerSpin = new QSpinBox();
                timerSpin->setMinimumWidth(88);
                timerSpin->setRange(0, 120);
                timerSpin->setSuffix(isEnglish ? QStringLiteral(" min") : QStringLiteral(" 分钟"));

                extraLayout->addWidget(modeLabel);
                extraLayout->addWidget(modeBox);
                extraLayout->addWidget(fanLabel);
                extraLayout->addWidget(fanBox);
                extraLayout->addWidget(timerLabel);
                extraLayout->addWidget(timerSpin);
                extraLayout->addStretch();
                bodyLayout->addWidget(extraPanel);
            }
            else if (!shouldHideExtraSettings(device) && device.type.contains(QStringLiteral("照明")))
            {
                QWidget *modePanel = new QWidget();
                modePanel->setAttribute(Qt::WA_StyledBackground, true);
                modePanel->setStyleSheet(panelStyle(palette));
                QHBoxLayout *modeLayout = new QHBoxLayout(modePanel);
                modeLayout->setContentsMargins(12, 8, 12, 8);
                modeLayout->setSpacing(10);

                QLabel *modeLabel = new QLabel(QStringLiteral("灯光模式"));
                modeLabel->setText(isEnglish ? QStringLiteral("Light Mode") : QStringLiteral("灯光模式"));
                modeLabel->setStyleSheet(mutedTextStyle(palette));
                modeLabel->setMinimumWidth(84);
                QComboBox *modeBox = new QComboBox();
                modeBox->setMinimumWidth(92);
                modeBox->addItems(isEnglish
                                      ? QStringList{QStringLiteral("Bright"), QStringLiteral("Warm"), QStringLiteral("Mixed")}
                                      : QStringList{QStringLiteral("亮色"), QStringLiteral("暖色"), QStringLiteral("混合")});
                modeLayout->addWidget(modeLabel);
                modeLayout->addWidget(modeBox);
                modeLayout->addStretch();
                bodyLayout->addWidget(modePanel);
            }
        }

        cardLayout->addLayout(bodyLayout);

        mainLayout->addWidget(deviceCard);
    }

    mainLayout->addStretch();

    // 删除顶部“当前分类”标签
    ui->label_categoryTitle->setVisible(false);

    if (ui->scrollArea->widget())
    {
        delete ui->scrollArea->widget();
    }
    ui->scrollArea->setWidget(contentWidget);
}

void DeviceControlWidget::updateDeviceStatus(const QJsonObject &statusData)
{
    const QString deviceId = statusData.value("device_id").toString().trimmed();
    if (deviceId.isEmpty())
    {
        qDebug() << "Ignore empty device status payload:" << statusData;
        return;
    }

    if (!m_deviceService.syncDeviceStatus(statusData))
    {
        qDebug() << "Failed to sync device status to database:" << statusData;
        return;
    }

    reloadDevices(false);
    updateDeviceListUI(ui->listCategory->currentRow());
}

void DeviceControlWidget::refreshDevices()
{
    reloadDevices(false);
    updateDeviceListUI(ui->listCategory->currentRow());
}

void DeviceControlWidget::scheduleThemeRefresh()
{
    if (!ui || m_themeRefreshScheduled)
    {
        return;
    }

    m_themeRefreshScheduled = true;
    QMetaObject::invokeMethod(this, [this]() {
        m_themeRefreshScheduled = false;
        if (!ui || !ui->listCategory)
        {
            return;
        }

        updateDeviceListUI(qMax(0, ui->listCategory->currentRow()));
    }, Qt::QueuedConnection);
}

void DeviceControlWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (!ui)
    {
        return;
    }

    if (event->type() == QEvent::PaletteChange
        || event->type() == QEvent::ApplicationPaletteChange
        || event->type() == QEvent::StyleChange)
    {
        if (ui->listCategory->count() > 0)
        {
            scheduleThemeRefresh();
        }
    }
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
