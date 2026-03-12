#include "devicecontrolwidget.h"
#include "ui_devicecontrolwidget.h"

#include "database/dao/EnvRecordDao.h"

#include <QApplication>
#include <QColor>
#include <QMetaObject>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QIcon>
#include <QHash>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QEvent>
#include <QPalette>
#include <QPainter>
#include <QPixmap>
#include <QScrollBar>
#include <QSlider>
#include <QToolButton>
#include <QVBoxLayout>
#include <QComboBox>
#include <QListWidgetItem>

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

    QString sensorValueStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor valueColor = dark ? blendColor(text, QColor("#FFFFFF"), 0.12)
                                       : QColor("#2A4D7A");
        return QStringLiteral("font-size: 11pt; font-weight: 700; color: %1;")
            .arg(colorCss(valueColor));
    }

    QString sensorMetricCellStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor bg = dark ? blendColor(window, QColor("#FFFFFF"), 0.08)
                               : QColor("#F4F8FD");
        const QColor border = dark ? blendColor(window, QColor("#FFFFFF"), 0.18)
                                   : QColor("#D7E4F2");
        return QStringLiteral("background: %1; border: 1px solid %2; border-radius: 8px;")
            .arg(colorCss(bg), colorCss(border));
    }

    QString sensorMetricValueStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor valueColor = dark ? blendColor(text, QColor("#FFFFFF"), 0.18)
                                       : QColor("#1F3F64");
        return QStringLiteral("font-size: 10pt; font-weight: 700; color: %1;")
            .arg(colorCss(valueColor));
    }

    QString timerValueButtonStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor muted = dark ? blendColor(text, window, 0.30) : blendColor(text, window, 0.48);
        return QStringLiteral(
                   "QPushButton { border: none; background: transparent; color: %1; font-size: 10pt; font-weight: 400; padding: 0 2px 0 0; text-align: left; }"
                   "QPushButton:hover { color: %2; }"
                   "QPushButton:pressed { color: %2; }")
            .arg(colorCss(text), colorCss(muted));
    }

    QString timerArrowButtonStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor blue = QColor("#007AFF");
        const QColor hoverBg = dark ? blendColor(window, QColor("#FFFFFF"), 0.12) : QColor("#E0E0E0");
        const QColor pressedBg = dark ? blendColor(window, QColor("#FFFFFF"), 0.18) : QColor("#D0D0D0");
        const QColor normalText = dark ? blendColor(text, window, 0.12) : QColor("#444444");
        return QStringLiteral(
                   "QToolButton { border: none; border-radius: 6px; background: transparent; color: %1; }"
                   "QToolButton:hover { background: %2; color: %3; }"
                   "QToolButton:pressed { background: %4; color: %3; }")
            .arg(colorCss(normalText), colorCss(hoverBg), colorCss(blue), colorCss(pressedBg));
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

    QString optionTagStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor bg = dark ? blendColor(window, QColor("#FFFFFF"), 0.12) : QColor("#EEF4FB");
        const QColor border = dark ? blendColor(window, QColor("#FFFFFF"), 0.24) : QColor("#C7D9EC");
        const QColor fg = dark ? blendColor(text, QColor("#FFFFFF"), 0.10) : QColor("#385B82");
        return QStringLiteral("color: %1; background: %2; border: 1px solid %3; border-radius: 10px; padding: 3px 10px; font-size: 9pt; font-weight: 600;")
            .arg(colorCss(fg), colorCss(bg), colorCss(border));
    }

    QString optionComboStyle(const QPalette &palette)
    {
        const bool dark = isDarkTheme(palette);
        const QColor window = palette.color(QPalette::Window);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor accent = palette.color(QPalette::Highlight);
        const QColor bg = dark ? blendColor(window, QColor("#FFFFFF"), 0.10) : QColor("#FFFFFF");
        const QColor border = dark ? blendColor(window, QColor("#FFFFFF"), 0.24) : QColor("#BFD4EA");
        const QColor hoverBorder = dark ? blendColor(accent, QColor("#FFFFFF"), 0.18) : blendColor(accent, QColor("#000000"), 0.12);
        const QColor viewBg = dark ? blendColor(window, QColor("#FFFFFF"), 0.08) : QColor("#FFFFFF");
        const QString downArrowIcon = dark
                                          ? QStringLiteral(":/icons/dropdown_arrow_light.svg")
                                          : QStringLiteral(":/icons/dropdown_arrow_dark.svg");
        return QStringLiteral(
                   "QComboBox { background: %1; color: %2; border: 1px solid %3; border-radius: 11px; padding: 5px 30px 5px 12px; min-height: 30px; font-size: 9pt; font-weight: 600; }"
                   "QComboBox:hover { border-color: %4; }"
                   "QComboBox:focus { border-color: %4; }"
                   "QComboBox::drop-down { border: none; width: 24px; }"
                   "QComboBox::down-arrow { image: url(%5); width: 12px; height: 8px; margin-right: 8px; }"
                   "QComboBox QAbstractItemView { background: %6; color: %2; border: 1px solid %3; selection-background-color: %4; selection-color: #FFFFFF; outline: none; }")
            .arg(colorCss(bg), colorCss(text), colorCss(border), colorCss(hoverBorder), downArrowIcon, colorCss(viewBg));
    }

    QPixmap themedDeviceIconPixmap(const QString &iconPath, const QPalette &palette)
    {
        const QIcon baseIcon(iconPath);
        QPixmap source = baseIcon.pixmap(28, 28);
        if (source.isNull())
        {
            return source;
        }

        if (!isDarkTheme(palette))
        {
            return source;
        }

        QPixmap tinted(source.size());
        tinted.fill(Qt::transparent);
        QPainter painter(&tinted);
        painter.drawPixmap(0, 0, source);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(tinted.rect(), QColor("#DCE7F8"));
        painter.end();
        return tinted;
    }

    bool isSwitchOnlySecurityDevice(const DeviceDefinition &device)
    {
        return device.id == QStringLiteral("lock_door") || device.id == QStringLiteral("camera_01") || device.name == QStringLiteral("前门智能锁") || device.name == QStringLiteral("客厅摄像头");
    }

    bool isVolumeOnlyTvDevice(const DeviceDefinition &device)
    {
        return device.id == QStringLiteral("tv_living") || device.name == QStringLiteral("客厅电视");
    }

    bool isLimitedCurtainDevice(const DeviceDefinition &device)
    {
        return device.id == QStringLiteral("curtain_bedroom") || device.name == QStringLiteral("卧室窗帘");
    }

    bool shouldHideExtraSettings(const DeviceDefinition &device)
    {
        return isSwitchOnlySecurityDevice(device) || isVolumeOnlyTvDevice(device) || isLimitedCurtainDevice(device);
    }

    bool isSensorDevice(const DeviceDefinition &device)
    {
        return device.type.contains(QStringLiteral("传感")) || device.name.contains(QStringLiteral("传感器"));
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
        if (device.type.contains(QStringLiteral("照明")) || device.type.contains(QStringLiteral("窗帘")) || device.type.contains(QStringLiteral("影音")) || device.name.contains(QStringLiteral("电视")))
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

    QString localizedDeviceName(const DeviceDefinition &device, bool isEnglish)
    {
        if (!isEnglish)
        {
            return device.name;
        }

        static const QHash<QString, QString> idMap = {
            {QStringLiteral("light_living"), QStringLiteral("Living Room Light")},
            {QStringLiteral("light_bedroom"), QStringLiteral("Bedroom Light")},
            {QStringLiteral("ac_living"), QStringLiteral("Living Room AC")},
            {QStringLiteral("curtain_living"), QStringLiteral("Living Room Curtain")},
            {QStringLiteral("lock_door"), QStringLiteral("Front Door Lock")},
            {QStringLiteral("camera_01"), QStringLiteral("Living Room Camera")},
            {QStringLiteral("tv_living"), QStringLiteral("Living Room TV")},
            {QStringLiteral("sensor_env"), QStringLiteral("Living Room Sensor")},
            {QStringLiteral("curtain_bedroom"), QStringLiteral("Bedroom Curtain")}};

        const QString byId = idMap.value(device.id);
        if (!byId.isEmpty())
        {
            return byId;
        }

        static const QHash<QString, QString> nameMap = {
            {QStringLiteral("客厅主灯"), QStringLiteral("Living Room Light")},
            {QStringLiteral("卧室灯"), QStringLiteral("Bedroom Light")},
            {QStringLiteral("客厅空调"), QStringLiteral("Living Room AC")},
            {QStringLiteral("客厅窗帘"), QStringLiteral("Living Room Curtain")},
            {QStringLiteral("前门智能锁"), QStringLiteral("Front Door Lock")},
            {QStringLiteral("客厅摄像头"), QStringLiteral("Living Room Camera")},
            {QStringLiteral("客厅电视"), QStringLiteral("Living Room TV")},
            {QStringLiteral("客厅环境传感器"), QStringLiteral("Living Room Sensor")},
            {QStringLiteral("卧室窗帘"), QStringLiteral("Bedroom Curtain")}};
        return nameMap.value(device.name, device.name);
    }

    QString categoryIconPath(const QString &category)
    {
        if (category.contains(QStringLiteral("照明")))
        {
            return QStringLiteral(":/icons/light.svg");
        }
        if (category.contains(QStringLiteral("空调")))
        {
            return QStringLiteral(":/icons/ac.svg");
        }
        if (category.contains(QStringLiteral("窗帘")))
        {
            return QStringLiteral(":/icons/curtains.svg");
        }
        if (category.contains(QStringLiteral("安防")))
        {
            return QStringLiteral(":/icons/lock.svg");
        }
        if (category.contains(QStringLiteral("影音")))
        {
            return QStringLiteral(":/icons/tv.svg");
        }
        if (category.contains(QStringLiteral("传感")))
        {
            return QStringLiteral(":/icons/warning.svg");
        }
        return QStringLiteral(":/icons/devices.svg");
    }

    QIcon categoryListIcon(const QString &category, const QPalette &palette)
    {
        const QString iconPath = categoryIconPath(category);
        QPixmap source(iconPath);
        if (source.isNull())
        {
            return QIcon(iconPath);
        }

        if (!isDarkTheme(palette))
        {
            return QIcon(source);
        }

        QPixmap tinted(source.size());
        tinted.fill(Qt::transparent);
        QPainter painter(&tinted);
        painter.drawPixmap(0, 0, source);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(tinted.rect(), QColor("#DCE7F8"));
        painter.end();
        return QIcon(tinted);
    }

    int indexForAcMode(const QString &modeCode)
    {
        const QString mode = modeCode.trimmed().toLower();
        if (mode == QStringLiteral("heat"))
        {
            return 1;
        }
        if (mode == QStringLiteral("dry"))
        {
            return 2;
        }
        if (mode == QStringLiteral("fan"))
        {
            return 3;
        }
        return 0;
    }

    QString acModeCodeByIndex(int index)
    {
        switch (index)
        {
        case 1:
            return QStringLiteral("heat");
        case 2:
            return QStringLiteral("dry");
        case 3:
            return QStringLiteral("fan");
        default:
            return QStringLiteral("cool");
        }
    }

    int indexForFanSpeed(const QString &speedCode)
    {
        const QString speed = speedCode.trimmed().toLower();
        if (speed == QStringLiteral("medium"))
        {
            return 1;
        }
        if (speed == QStringLiteral("high"))
        {
            return 2;
        }
        return 0;
    }

    QString fanSpeedCodeByIndex(int index)
    {
        switch (index)
        {
        case 1:
            return QStringLiteral("medium");
        case 2:
            return QStringLiteral("high");
        default:
            return QStringLiteral("low");
        }
    }

    int indexForLightMode(const QVariantMap &extraParams)
    {
        const QString mode = extraParams.value(QStringLiteral("light_mode")).toString().trimmed().toLower();
        if (mode == QStringLiteral("warm"))
        {
            return 1;
        }
        if (mode == QStringLiteral("mixed"))
        {
            return 2;
        }
        if (mode == QStringLiteral("bright"))
        {
            return 0;
        }

        const int colorTemp = extraParams.value(QStringLiteral("color_temp"), 4500).toInt();
        if (colorTemp <= 3600)
        {
            return 1;
        }
        if (colorTemp >= 5000)
        {
            return 0;
        }
        return 2;
    }

    QString lightModeCodeByIndex(int index)
    {
        switch (index)
        {
        case 1:
            return QStringLiteral("warm");
        case 2:
            return QStringLiteral("mixed");
        default:
            return QStringLiteral("bright");
        }
    }

    QString buildDeviceRenderSignature(const DeviceList &devices)
    {
        QString signature;
        signature.reserve(devices.size() * 48);
        for (const DeviceDefinition &device : devices)
        {
            signature += device.id;
            signature += QLatin1Char('|');
            signature += device.type;
            signature += QLatin1Char('|');
            signature += device.isOnline ? QLatin1Char('1') : QLatin1Char('0');
            signature += QLatin1Char('|');
            signature += device.isOn ? QLatin1Char('1') : QLatin1Char('0');
            signature += QLatin1Char('|');
            signature += QString::number(device.value);
            signature += QLatin1Char('|');
            signature += device.valueUnit;
            signature += QLatin1Char(';');
        }
        return signature;
    }
}

DeviceControlWidget::DeviceControlWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::DeviceControlWidget)
{
    ui->setupUi(this);
    ui->listCategory->setIconSize(QSize(20, 20));
    ui->listCategory->setSpacing(6);
    applyLanguage(QStringLiteral("zh_CN"));
    m_categories = m_deviceService.categories();
    initDeviceList();

    connect(&m_deviceService, &DeviceService::devicesRefreshed,
            this, &DeviceControlWidget::onDevicesRefreshed);
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
    const QPalette pal = this->palette();
    for (const QString &category : m_categories)
    {
        QListWidgetItem *item = new QListWidgetItem(categoryListIcon(category, pal), localizedCategory(category, isEnglish));
        item->setSizeHint(QSize(item->sizeHint().width(), 56));
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        ui->listCategory->addItem(item);
    }
    ui->listCategory->setCurrentRow(qMin(currentRow, ui->listCategory->count() - 1));
    updateDeviceListUI(ui->listCategory->currentRow());
}

void DeviceControlWidget::initDeviceList()
{
    ui->listCategory->clear();
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    const QPalette pal = this->palette();
    for (const QString &category : m_categories)
    {
        QListWidgetItem *item = new QListWidgetItem(categoryListIcon(category, pal), localizedCategory(category, isEnglish));
        item->setSizeHint(QSize(item->sizeHint().width(), 56));
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        ui->listCategory->addItem(item);
    }
    ui->listCategory->setCurrentRow(0);
    updateDeviceListUI(0);
}

void DeviceControlWidget::updateDeviceListUI(int category)
{
    if (!ui || !ui->label_categoryTitle || !ui->scrollArea)
    {
        return;
    }

    const int normalizedCategory = qMax(0, category);
    const DeviceList filteredDevices = m_deviceService.filterDevices(m_allDevices, normalizedCategory, m_categories);
    const QPalette palette = this->palette();
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    const QString filteredSignature = buildDeviceRenderSignature(filteredDevices) +
                                      QStringLiteral("|lang=") + m_languageKey +
                                      QStringLiteral("|win=") + QString::number(palette.color(QPalette::Window).rgba(), 16);

    if (normalizedCategory == m_lastRenderedCategory && filteredSignature == m_lastDeviceRenderSignature)
    {
        return;
    }

    int preservedScrollValue = 0;
    if (ui && ui->scrollArea && ui->scrollArea->verticalScrollBar())
    {
        preservedScrollValue = ui->scrollArea->verticalScrollBar()->value();
    }

    QWidget *contentWidget = new QWidget();
    contentWidget->setAttribute(Qt::WA_StyledBackground, true);
    contentWidget->setStyleSheet(transparentWidgetStyle());
    QVBoxLayout *mainLayout = new QVBoxLayout(contentWidget);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(8, 8, 8, 8);

    QStringList filteredIds;
    filteredIds.reserve(filteredDevices.size());
    for (const DeviceDefinition &device : filteredDevices)
    {
        filteredIds.push_back(device.id);
    }
    const QHash<QString, QVariantMap> extraParamsMap = m_deviceService.loadExtraParamsBatch(filteredIds);
    std::optional<EnvRealtimeSnapshot> envSnapshot;
    bool envSnapshotFetched = false;

    auto refreshCurrentCategory = [this]()
    {
        m_deviceService.refreshNow();
    };

    auto deferAutoRefresh = [this]()
    {
        m_deviceService.stopPolling();
        const int seq = this->property("interactionSeq").toInt() + 1;
        this->setProperty("interactionSeq", seq);
        QTimer::singleShot(1400, this, [this, seq]()
                           {
            if (this->property("interactionSeq").toInt() != seq)
            {
                return;
            }

            if (isVisible())
            {
                m_deviceService.startPolling(5000);
            } });
    };

    auto createSwitchButton = [this, &refreshCurrentCategory, palette, isEnglish](const DeviceDefinition &device)
    {
        QPushButton *switchBtn = new QPushButton(actionTextForDevice(device, isEnglish));
        switchBtn->setCursor(Qt::PointingHandCursor);
        switchBtn->setFixedWidth(116);
        switchBtn->setMinimumHeight(34);
        switchBtn->setStyleSheet(actionStyleForDevice(device, palette));
        connect(switchBtn, &QPushButton::clicked, this, [this, device, refreshCurrentCategory]()
                {
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
            } });
        return switchBtn;
    };

    auto addSliderControl = [this, &refreshCurrentCategory, palette](QVBoxLayout *targetLayout,
                                                                     const DeviceDefinition &device,
                                                                     const QString &labelText,
                                                                     int minValue,
                                                                     int maxValue,
                                                                     const auto &valueFormatter)
    {
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
        connect(slider, &QSlider::valueChanged, this, [valueLabel, valueFormatter](int value)
                { valueLabel->setText(valueFormatter(value)); });
        connect(slider, &QSlider::sliderReleased, this, [this, slider, device, refreshCurrentCategory]()
                {
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
            } });
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
        iconLabel->setPixmap(themedDeviceIconPixmap(device.icon, palette));
        iconLabel->setFixedSize(32, 32);
        headerLayout->addWidget(iconLabel, 0, Qt::AlignTop);

        QWidget *infoWidget = new QWidget();
        infoWidget->setAttribute(Qt::WA_StyledBackground, true);
        infoWidget->setStyleSheet(transparentWidgetStyle());
        QVBoxLayout *infoLayout = new QVBoxLayout(infoWidget);
        infoLayout->setContentsMargins(0, 0, 0, 0);
        infoLayout->setSpacing(6);

        QLabel *nameLabel = new QLabel(localizedDeviceName(device, isEnglish));
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
            QString readingText = localizedValueText(device, device.value, isEnglish);
            if (!envSnapshotFetched)
            {
                EnvRecordDao envRecordDao;
                envSnapshot = envRecordDao.getLatestRealtimeSnapshot();
                envSnapshotFetched = true;
            }

            const bool useHomeSnapshot = envSnapshot.has_value() && (device.id.contains(QStringLiteral("living"), Qt::CaseInsensitive) || device.name.contains(QStringLiteral("客厅")));
            if (useHomeSnapshot)
            {
                const int displayTemp = qRound(envSnapshot->temperature);
                readingText = isEnglish
                                  ? QStringLiteral("%1 °C").arg(displayTemp)
                                  : QStringLiteral("%1°C").arg(displayTemp);
            }

            QWidget *readingPanel = new QWidget();
            readingPanel->setAttribute(Qt::WA_StyledBackground, true);
            readingPanel->setStyleSheet(panelStyle(palette));
            QVBoxLayout *readingLayout = new QVBoxLayout(readingPanel);
            readingLayout->setContentsMargins(12, 8, 12, 10);
            readingLayout->setSpacing(8);

            QHBoxLayout *panelHeaderLayout = new QHBoxLayout();
            panelHeaderLayout->setContentsMargins(0, 0, 0, 0);
            panelHeaderLayout->setSpacing(8);

            QLabel *readingTag = new QLabel(isEnglish ? QStringLiteral("Live Data") : QStringLiteral("实时数据"));
            readingTag->setStyleSheet(optionTagStyle(palette));
            readingTag->setMinimumWidth(64);
            panelHeaderLayout->addWidget(readingTag, 0, Qt::AlignVCenter);

            QLabel *readingHint = new QLabel(isEnglish ? QStringLiteral("Sensor metrics") : QStringLiteral("传感器指标"));
            readingHint->setStyleSheet(mutedTextStyle(palette, 9, 500));
            panelHeaderLayout->addWidget(readingHint, 0, Qt::AlignVCenter);
            panelHeaderLayout->addStretch();
            readingLayout->addLayout(panelHeaderLayout);

            QGridLayout *metricsLayout = new QGridLayout();
            metricsLayout->setContentsMargins(0, 0, 0, 0);
            metricsLayout->setHorizontalSpacing(8);
            metricsLayout->setVerticalSpacing(8);

            auto addMetricCell = [&](int row, int col, const QString &metricName, const QString &metricValue)
            {
                QWidget *metricCell = new QWidget();
                metricCell->setAttribute(Qt::WA_StyledBackground, true);
                metricCell->setStyleSheet(sensorMetricCellStyle(palette));

                QVBoxLayout *metricLayout = new QVBoxLayout(metricCell);
                metricLayout->setContentsMargins(10, 7, 10, 7);
                metricLayout->setSpacing(2);

                QLabel *nameLabel = new QLabel(metricName);
                nameLabel->setStyleSheet(mutedTextStyle(palette, 9, 600));

                QLabel *valueLabel = new QLabel(metricValue);
                valueLabel->setStyleSheet(sensorMetricValueStyle(palette));

                metricLayout->addWidget(nameLabel);
                metricLayout->addWidget(valueLabel);
                metricsLayout->addWidget(metricCell, row, col);
            };

            if (useHomeSnapshot)
            {
                const QString temperatureText = isEnglish
                                                    ? QStringLiteral("%1 °C").arg(QString::number(envSnapshot->temperature, 'f', 1))
                                                    : QStringLiteral("%1°C").arg(QString::number(envSnapshot->temperature, 'f', 1));
                const QString humidityText = QStringLiteral("%1%").arg(QString::number(envSnapshot->humidity, 'f', 1));

                addMetricCell(0, 0, isEnglish ? QStringLiteral("Temperature") : QStringLiteral("温度"), temperatureText);
                addMetricCell(0, 1, isEnglish ? QStringLiteral("Humidity") : QStringLiteral("湿度"), humidityText);
            }
            else
            {
                addMetricCell(0, 0, isEnglish ? QStringLiteral("Current reading") : QStringLiteral("当前读数"), readingText);

                QLabel *readingValueLabel = new QLabel((isEnglish ? QStringLiteral("Primary reading: ") : QStringLiteral("主读数：")) + readingText);
                readingValueLabel->setStyleSheet(sensorValueStyle(palette));
                readingLayout->addWidget(readingValueLabel, 0, Qt::AlignLeft);
            }

            readingLayout->addLayout(metricsLayout);

            cardLayout->addWidget(readingPanel);
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
            addSliderControl(bodyLayout, device, isEnglish ? QStringLiteral("Volume") : QStringLiteral("音量"), 0, 100, [](int value)
                             { return QString::number(value) + QStringLiteral("%"); });
        }
        else if (isLimitedCurtain)
        {
            addSliderControl(bodyLayout, device, isEnglish ? QStringLiteral("Open Level") : QStringLiteral("开合度"), 0, 100, [](int value)
                             { return QString::number(value) + QStringLiteral("%"); });
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
                                 [device, isEnglish](int value)
                                 {
                                     return localizedValueText(device, value, isEnglish);
                                 });
            }

            if (!shouldHideExtraSettings(device) && device.type.contains(QStringLiteral("空调")))
            {
                const QVariantMap extraParams = extraParamsMap.value(device.id);
                QWidget *extraPanel = new QWidget();
                extraPanel->setAttribute(Qt::WA_StyledBackground, true);
                extraPanel->setStyleSheet(panelStyle(palette));
                QHBoxLayout *extraLayout = new QHBoxLayout(extraPanel);
                extraLayout->setContentsMargins(12, 8, 12, 8);
                extraLayout->setSpacing(10);

                QLabel *modeLabel = new QLabel(QStringLiteral("模式"));
                modeLabel->setText(isEnglish ? QStringLiteral("Mode") : QStringLiteral("模式"));
                modeLabel->setStyleSheet(optionTagStyle(palette));
                modeLabel->setMinimumWidth(44);
                QComboBox *modeBox = new QComboBox();
                modeBox->setStyleSheet(optionComboStyle(palette));
                modeBox->setMinimumWidth(84);
                modeBox->addItems(isEnglish
                                      ? QStringList{QStringLiteral("Cool"), QStringLiteral("Heat"), QStringLiteral("Dry"), QStringLiteral("Fan")}
                                      : QStringList{QStringLiteral("制冷"), QStringLiteral("制热"), QStringLiteral("除湿"), QStringLiteral("送风")});
                modeBox->setCurrentIndex(indexForAcMode(extraParams.value(QStringLiteral("ac_mode"), QStringLiteral("cool")).toString()));

                QLabel *fanLabel = new QLabel(QStringLiteral("风速"));
                fanLabel->setText(isEnglish ? QStringLiteral("Fan") : QStringLiteral("风速"));
                fanLabel->setStyleSheet(optionTagStyle(palette));
                fanLabel->setMinimumWidth(36);
                QComboBox *fanBox = new QComboBox();
                fanBox->setStyleSheet(optionComboStyle(palette));
                fanBox->setMinimumWidth(72);
                fanBox->addItems(isEnglish
                                     ? QStringList{QStringLiteral("Low"), QStringLiteral("Medium"), QStringLiteral("High")}
                                     : QStringList{QStringLiteral("低"), QStringLiteral("中"), QStringLiteral("高")});
                fanBox->setCurrentIndex(indexForFanSpeed(extraParams.value(QStringLiteral("fan_speed"), QStringLiteral("low")).toString()));

                QLabel *timerLabel = new QLabel(QStringLiteral("定时"));
                timerLabel->setText(isEnglish ? QStringLiteral("Timer") : QStringLiteral("定时"));
                timerLabel->setStyleSheet(optionTagStyle(palette));
                timerLabel->setMinimumWidth(44);
                QWidget *timerEditor = new QWidget();
                timerEditor->setAttribute(Qt::WA_StyledBackground, true);
                timerEditor->setStyleSheet(panelStyle(palette));
                timerEditor->setMinimumWidth(126);
                timerEditor->setMinimumHeight(44);
                QHBoxLayout *timerLayout = new QHBoxLayout(timerEditor);
                timerLayout->setContentsMargins(10, 4, 6, 4);
                timerLayout->setSpacing(3);

                QPushButton *timerValueButton = new QPushButton();
                timerValueButton->setCursor(Qt::PointingHandCursor);
                timerValueButton->setStyleSheet(timerValueButtonStyle(palette));

                QWidget *arrowColumn = new QWidget();
                arrowColumn->setAttribute(Qt::WA_StyledBackground, true);
                arrowColumn->setStyleSheet(transparentWidgetStyle());
                QVBoxLayout *arrowLayout = new QVBoxLayout(arrowColumn);
                arrowLayout->setContentsMargins(0, 0, 0, 0);
                arrowLayout->setSpacing(2);

                QToolButton *upButton = new QToolButton();
                upButton->setArrowType(Qt::UpArrow);
                upButton->setFixedSize(24, 18);
                upButton->setAutoRepeat(true);
                upButton->setAutoRepeatInterval(90);
                upButton->setCursor(Qt::PointingHandCursor);
                upButton->setAutoRaise(true);
                upButton->setStyleSheet(timerArrowButtonStyle(palette));

                QToolButton *downButton = new QToolButton();
                downButton->setArrowType(Qt::DownArrow);
                downButton->setFixedSize(24, 18);
                downButton->setAutoRepeat(true);
                downButton->setAutoRepeatInterval(90);
                downButton->setCursor(Qt::PointingHandCursor);
                downButton->setAutoRaise(true);
                downButton->setStyleSheet(timerArrowButtonStyle(palette));

                arrowLayout->addWidget(upButton);
                arrowLayout->addWidget(downButton);

                const int timerInit = qBound(0, extraParams.value(QStringLiteral("timer_minutes"), 0).toInt(), 120);
                timerEditor->setProperty("timerMinutes", timerInit);
                timerValueButton->setText(isEnglish ? QStringLiteral("%1 min").arg(timerInit) : QStringLiteral("%1 分钟").arg(timerInit));

                auto persistTimerMinutes = [this, device, deferAutoRefresh](int minutes)
                {
                    deferAutoRefresh();
                    QString errorMessage;
                    QString warningMessage;
                    if (!m_deviceService.updateExtraParam(device.id,
                                                          QStringLiteral("timer_minutes"),
                                                          minutes,
                                                          QStringLiteral("空调定时"),
                                                          QStringLiteral("min"),
                                                          &errorMessage,
                                                          &warningMessage))
                    {
                        QMessageBox::critical(this, kFailed, errorMessage.trimmed().isEmpty() ? QStringLiteral("空调定时写入数据库失败。") : errorMessage);
                        return;
                    }
                    if (!warningMessage.trimmed().isEmpty())
                    {
                        QMessageBox::warning(this, kWarning, warningMessage);
                    }
                };
                auto updateTimerValue = [timerEditor, timerValueButton, isEnglish, persistTimerMinutes](int deltaMinutes)
                {
                    const int current = timerEditor->property("timerMinutes").toInt();
                    const int next = qBound(0, current + deltaMinutes, 120);
                    if (next == current)
                    {
                        return;
                    }
                    timerEditor->setProperty("timerMinutes", next);
                    timerValueButton->setText(isEnglish ? QStringLiteral("%1 min").arg(next) : QStringLiteral("%1 分钟").arg(next));
                    persistTimerMinutes(next);
                };

                connect(upButton, &QToolButton::clicked, this, [updateTimerValue]()
                        { updateTimerValue(30); });
                connect(downButton, &QToolButton::clicked, this, [updateTimerValue]()
                        { updateTimerValue(-30); });

                connect(timerValueButton, &QPushButton::clicked, this, [this, timerEditor, timerValueButton, isEnglish, persistTimerMinutes]()
                        {
                    const int current = timerEditor->property("timerMinutes").toInt();
                    bool ok = false;
                    const int value = QInputDialog::getInt(this,
                                                           isEnglish ? QStringLiteral("Set Timer") : QStringLiteral("设置定时"),
                                                           isEnglish ? QStringLiteral("Minutes (30 min step)") : QStringLiteral("分钟（30 分钟步进）"),
                                                           current,
                                                           0,
                                                           120,
                                                           30,
                                                           &ok);
                    if (!ok)
                    {
                        return;
                    }

                    timerEditor->setProperty("timerMinutes", value);
                    timerValueButton->setText(isEnglish ? QStringLiteral("%1 min").arg(value) : QStringLiteral("%1 分钟").arg(value));
                    persistTimerMinutes(value); });

                timerLayout->addWidget(timerValueButton, 1);
                timerLayout->addWidget(arrowColumn, 0, Qt::AlignRight | Qt::AlignVCenter);

                connect(modeBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, device, modeBox, deferAutoRefresh]()
                        {
                    deferAutoRefresh();
                    QString errorMessage;
                    QString warningMessage;
                    const QString modeCode = acModeCodeByIndex(modeBox->currentIndex());
                    if (!m_deviceService.updateExtraParam(device.id,
                                                          QStringLiteral("ac_mode"),
                                                          modeCode,
                                                          QStringLiteral("空调模式"),
                                                          QString(),
                                                          &errorMessage,
                                                          &warningMessage))
                    {
                        QMessageBox::critical(this, kFailed, errorMessage.trimmed().isEmpty() ? QStringLiteral("空调模式写入数据库失败。") : errorMessage);
                        return;
                    }
                    if (!warningMessage.trimmed().isEmpty())
                    {
                        QMessageBox::warning(this, kWarning, warningMessage);
                    } });

                connect(fanBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, device, fanBox, deferAutoRefresh]()
                        {
                    deferAutoRefresh();
                    QString errorMessage;
                    QString warningMessage;
                    const QString speedCode = fanSpeedCodeByIndex(fanBox->currentIndex());
                    if (!m_deviceService.updateExtraParam(device.id,
                                                          QStringLiteral("fan_speed"),
                                                          speedCode,
                                                          QStringLiteral("空调风速"),
                                                          QString(),
                                                          &errorMessage,
                                                          &warningMessage))
                    {
                        QMessageBox::critical(this, kFailed, errorMessage.trimmed().isEmpty() ? QStringLiteral("空调风速写入数据库失败。") : errorMessage);
                        return;
                    }
                    if (!warningMessage.trimmed().isEmpty())
                    {
                        QMessageBox::warning(this, kWarning, warningMessage);
                    } });

                extraLayout->addWidget(modeLabel);
                extraLayout->addWidget(modeBox);
                extraLayout->addWidget(fanLabel);
                extraLayout->addWidget(fanBox);
                extraLayout->addWidget(timerLabel);
                extraLayout->addWidget(timerEditor);
                extraLayout->addStretch();
                bodyLayout->addWidget(extraPanel);
            }
            else if (!shouldHideExtraSettings(device) && device.type.contains(QStringLiteral("照明")))
            {
                const QVariantMap extraParams = extraParamsMap.value(device.id);
                QWidget *modePanel = new QWidget();
                modePanel->setAttribute(Qt::WA_StyledBackground, true);
                modePanel->setStyleSheet(panelStyle(palette));
                QHBoxLayout *modeLayout = new QHBoxLayout(modePanel);
                modeLayout->setContentsMargins(12, 8, 12, 8);
                modeLayout->setSpacing(10);

                QLabel *modeLabel = new QLabel(isEnglish ? QStringLiteral("Light Mode") : QStringLiteral("灯光模式"));
                modeLabel->setStyleSheet(optionTagStyle(palette));
                modeLabel->setMinimumWidth(84);

                QComboBox *modeBox = new QComboBox();
                modeBox->setStyleSheet(optionComboStyle(palette));
                modeBox->setMinimumWidth(96);
                modeBox->addItems(isEnglish
                                      ? QStringList{QStringLiteral("Bright"), QStringLiteral("Warm"), QStringLiteral("Mixed")}
                                      : QStringList{QStringLiteral("亮色"), QStringLiteral("暖色"), QStringLiteral("混合")});
                modeBox->setCurrentIndex(indexForLightMode(extraParams));

                connect(modeBox, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, device, modeBox, deferAutoRefresh]()
                        {
                    deferAutoRefresh();
                    QString errorMessage;
                    QString warningMessage;

                    const QString modeCode = lightModeCodeByIndex(modeBox->currentIndex());
                    if (!m_deviceService.updateExtraParam(device.id,
                                                          QStringLiteral("light_mode"),
                                                          modeCode,
                                                          QStringLiteral("灯光模式"),
                                                          QString(),
                                                          &errorMessage,
                                                          &warningMessage))
                    {
                        QMessageBox::critical(this, kFailed, errorMessage.trimmed().isEmpty() ? QStringLiteral("灯光模式写入数据库失败。") : errorMessage);
                        return;
                    }
                    if (!warningMessage.trimmed().isEmpty())
                    {
                        QMessageBox::warning(this, kWarning, warningMessage);
                    } });

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
    if (ui->scrollArea->verticalScrollBar())
    {
        QScrollBar *scrollBar = ui->scrollArea->verticalScrollBar();
        QTimer::singleShot(0, this, [scrollBar, preservedScrollValue]()
                           {
            if (scrollBar)
            {
                scrollBar->setValue(preservedScrollValue);
            } });
    }

    m_lastRenderedCategory = normalizedCategory;
    m_lastDeviceRenderSignature = filteredSignature;
}

void DeviceControlWidget::refreshDevices()
{
    m_deviceService.refreshNow();
}

void DeviceControlWidget::scheduleThemeRefresh()
{
    if (!ui || m_themeRefreshScheduled)
    {
        return;
    }

    m_themeRefreshScheduled = true;
    QMetaObject::invokeMethod(this, [this]()
                              {
        m_themeRefreshScheduled = false;
        if (!ui || !ui->listCategory)
        {
            return;
        }

        const int currentRow = qMax(0, ui->listCategory->currentRow());
        const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
        const QPalette pal = this->palette();

        ui->listCategory->blockSignals(true);
        ui->listCategory->clear();
        for (const QString &category : m_categories)
        {
            QListWidgetItem *item = new QListWidgetItem(categoryListIcon(category, pal), localizedCategory(category, isEnglish));
            item->setSizeHint(QSize(item->sizeHint().width(), 56));
            item->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
            ui->listCategory->addItem(item);
        }
        ui->listCategory->setCurrentRow(qMin(currentRow, ui->listCategory->count() - 1));
        ui->listCategory->blockSignals(false);

        updateDeviceListUI(qMax(0, ui->listCategory->currentRow())); }, Qt::QueuedConnection);
}

void DeviceControlWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (!ui)
    {
        return;
    }

    if (event->type() == QEvent::PaletteChange || event->type() == QEvent::ApplicationPaletteChange || event->type() == QEvent::StyleChange)
    {
        scheduleThemeRefresh();
    }
}

void DeviceControlWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_deviceService.startPolling(5000);
}

void DeviceControlWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_deviceService.stopPolling();
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

void DeviceControlWidget::onDevicesRefreshed(DeviceList devices)
{
    m_allDevices = devices;
    updateDeviceListUI(ui && ui->listCategory ? qMax(0, ui->listCategory->currentRow()) : 0);
}
