#include "homewidget.h"
#include "quickcontrolmanagedialog.h"
#include "ui_homewidget.h"

#include <QDebug>
#include <QColor>
#include <QEvent>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QMetaObject>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QTimer>
#include <QToolButton>

namespace
{
    bool isDarkTheme(const QPalette &palette)
    {
        return palette.color(QPalette::Window).lightness() < 128;
    }

    QColor mixColor(const QColor &a, const QColor &b, qreal ratio)
    {
        const qreal r = qBound<qreal>(0.0, ratio, 1.0);
        return QColor::fromRgbF(a.redF() * (1.0 - r) + b.redF() * r,
                                a.greenF() * (1.0 - r) + b.greenF() * r,
                                a.blueF() * (1.0 - r) + b.blueF() * r,
                                1.0);
    }

    QString cssColor(const QColor &color)
    {
        return color.name(QColor::HexRgb);
    }

    QIcon tintedIcon(const QString &path, const QColor &color, const QSize &targetSize)
    {
        const QIcon baseIcon(path);
        const QSize safeSize = targetSize.isValid() ? targetSize : QSize(40, 40);
        const QPixmap source = baseIcon.pixmap(safeSize);
        if (source.isNull())
        {
            return baseIcon;
        }

        QPixmap tinted(safeSize);
        tinted.fill(Qt::transparent);
        QPainter painter(&tinted);
        const QRect targetRect(QPoint(0, 0), safeSize);
        const QPixmap scaled = source.scaled(safeSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        const int iconShiftX = 8;
        const QPoint offset((safeSize.width() - scaled.width()) / 2 + iconShiftX,
                            (safeSize.height() - scaled.height()) / 2);
        painter.drawPixmap(offset, scaled);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(targetRect, color);
        painter.end();
        return QIcon(tinted);
    }

    QString quickControlCardStyle(const QPalette &palette,
                                  bool singleRowLayout,
                                  bool isOffline,
                                  bool isSelected,
                                  bool isEnabled)
    {
        const bool dark = isDarkTheme(palette);
        const QColor base = palette.color(QPalette::Base);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor accent = palette.color(QPalette::Highlight);

        QColor background = dark ? mixColor(base, QColor("#FFFFFF"), 0.13) : QColor("#F7FAFD");
        QColor border = dark ? mixColor(base, QColor("#FFFFFF"), 0.28) : QColor("#DCE6F2");
        QColor hover = dark ? mixColor(background, QColor("#FFFFFF"), 0.10) : QColor("#EAF3FF");
        QColor textColor = dark ? QColor("#E4ECF7") : QColor("#2F4258");

        if (isSelected)
        {
            background = dark ? mixColor(accent, base, 0.52) : QColor("#DDEEFF");
            border = dark ? mixColor(accent, QColor("#FFFFFF"), 0.40) : QColor("#2D8CFF");
            hover = dark ? mixColor(background, QColor("#FFFFFF"), 0.16) : mixColor(background, QColor("#000000"), 0.08);
            textColor = dark ? QColor("#F1F7FF") : QColor("#1F5FA8");
        }
        else if (isOffline || !isEnabled)
        {
            background = dark ? mixColor(base, QColor("#FFFFFF"), 0.06) : QColor("#F2F4F8");
            border = dark ? mixColor(base, QColor("#FFFFFF"), 0.16) : QColor("#D2D9E2");
            hover = background;
            textColor = dark ? mixColor(text, base, 0.35) : QColor("#8F99A4");
        }

        const QString paddingStyle = singleRowLayout ? QStringLiteral("padding: 16px;") : QStringLiteral("padding: 8px;");
        return QStringLiteral("QToolButton { background: %1; border: 1px solid %2; border-radius: 14px; color: %3; font-size: 11pt; font-weight: 800; text-align: center; outline: none; %4 }"
                              "QToolButton:hover { background: %5; }"
                              "QToolButton:focus { outline: none; border: 1px solid %2; }"
                              "QToolButton:disabled { background: %6; border-color: %7; color: %8; }")
            .arg(cssColor(background),
                 cssColor(border),
                 cssColor(textColor),
                 paddingStyle,
                 cssColor(hover),
                 cssColor(dark ? mixColor(base, QColor("#FFFFFF"), 0.05) : QColor("#EEF1F5")),
                 cssColor(dark ? mixColor(base, QColor("#FFFFFF"), 0.12) : QColor("#D3D9E1")),
                 cssColor(dark ? mixColor(text, base, 0.42) : QColor("#98A2AD")));
    }

    QColor quickCardIconColor(const QPalette &palette, bool isSelected, bool isOffline, bool isEnabled)
    {
        const bool dark = isDarkTheme(palette);
        const QColor text = palette.color(QPalette::WindowText);
        const QColor accent = palette.color(QPalette::Highlight);
        if (isSelected)
        {
            return dark ? mixColor(accent, QColor("#FFFFFF"), 0.28) : QColor("#1F73D1");
        }
        if (isOffline || !isEnabled)
        {
            return dark ? mixColor(text, QColor("#111827"), 0.55) : QColor("#9098A2");
        }
        return dark ? mixColor(text, QColor("#FFFFFF"), 0.18) : QColor("#2F6FAF");
    }

    QString localizedQuickControlName(const QString &name, bool isEnglish)
    {
        if (!isEnglish)
        {
            return name;
        }

        static const QHash<QString, QString> map = {
            {QStringLiteral("客厅主灯"), QStringLiteral("Living Room Light")},
            {QStringLiteral("卧室灯"), QStringLiteral("Bedroom Light")},
            {QStringLiteral("客厅空调"), QStringLiteral("Living Room AC")},
            {QStringLiteral("客厅窗帘"), QStringLiteral("Living Room Curtain")},
            {QStringLiteral("前门智能锁"), QStringLiteral("Front Door Lock")},
            {QStringLiteral("客厅摄像头"), QStringLiteral("Living Room Camera")},
            {QStringLiteral("客厅电视"), QStringLiteral("Living Room TV")},
            {QStringLiteral("客厅环境传感器"), QStringLiteral("Living Room Sensor")},
            {QStringLiteral("回家模式"), QStringLiteral("Home Mode")},
            {QStringLiteral("睡眠模式"), QStringLiteral("Sleep Mode")},
            {QStringLiteral("观影模式"), QStringLiteral("Movie Mode")},
            {QStringLiteral("离家模式"), QStringLiteral("Away Mode")}};
        return map.value(name, name);
    }
}

HomeWidget::HomeWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
    ui->label_title->setStyleSheet(QStringLiteral("font-size: 18pt; font-weight: 700;"));
    ui->label_temperature->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight: 700;"));
    ui->label_humidity->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight: 700;"));
    ui->label_deviceCount->setStyleSheet(QStringLiteral("font-size: 22pt; font-weight: 700;"));
    ensureQuickControlEditButton();
    applyLanguage(QStringLiteral("zh_CN"));

    connect(&m_environmentService, &EnvironmentService::snapshotRefreshed,
            this, &HomeWidget::onEnvironmentSnapshotLoaded);

    m_quickControlResizeTimer = new QTimer(this);
    m_quickControlResizeTimer->setSingleShot(true);
    m_quickControlResizeTimer->setInterval(150);
    connect(m_quickControlResizeTimer, &QTimer::timeout, this, [this]()
            {
        if (!isVisible())
        {
            return;
        }

        const int columnsNow = quickControlColumnsForWidth(ui->quickControlContainer ? ui->quickControlContainer->width() : width());
        if (columnsNow != m_lastQuickControlColumns)
        {
            refreshQuickControls();
        } });

    // 轮询由 showEvent 启动，hideEvent 停止
}

HomeWidget::~HomeWidget()
{
    delete ui;
}

void HomeWidget::applyLanguage(const QString &languageKey)
{
    if (languageKey.trimmed().isEmpty())
    {
        return;
    }

    m_languageKey = languageKey;
    refreshStaticTexts();
    refreshDeviceStatus();
}

void HomeWidget::initConnections()
{
}

void HomeWidget::ensureQuickControlEditButton()
{
    m_editQuickControlButton = findChild<QPushButton *>(QStringLiteral("btnEditQuickControl"));
    if (!m_editQuickControlButton)
    {
        m_editQuickControlButton = new QPushButton(QStringLiteral("编辑快捷控制"), this);
        m_editQuickControlButton->setObjectName(QStringLiteral("btnEditQuickControl"));

        QHBoxLayout *actionLayout = new QHBoxLayout();
        actionLayout->setContentsMargins(0, 0, 0, 0);
        actionLayout->setSpacing(0);
        actionLayout->addStretch();
        actionLayout->addWidget(m_editQuickControlButton, 0, Qt::AlignRight);
        ui->verticalLayout_quick->insertLayout(0, actionLayout);
    }

    m_editQuickControlButton->setText(m_languageKey == QStringLiteral("en_US")
                                          ? QStringLiteral("Edit Shortcuts")
                                          : QStringLiteral("编辑快捷控制"));
    m_editQuickControlButton->setCursor(Qt::PointingHandCursor);
    m_editQuickControlButton->setMinimumHeight(32);
    m_editQuickControlButton->setMinimumWidth(140);
    m_editQuickControlButton->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_editQuickControlButton->setVisible(true);
    connect(m_editQuickControlButton, &QPushButton::clicked, this, &HomeWidget::on_btnEditQuickControl_clicked, Qt::UniqueConnection);
}

void HomeWidget::onQuickControlClicked()
{
    qDebug() << "快捷控制被点击";
}

void HomeWidget::on_btnEditQuickControl_clicked()
{
    QuickControlManageDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        loadQuickControls();
    }
}

void HomeWidget::loadQuickControls()
{
    const QList<QuickControlDisplayItem> items = m_quickControlService.getHomeShortcuts();
    const int preferredCardWidth = 220;
    const int containerWidth = qMax(1, ui->quickControlContainer->width());
    const int maxColumns = qBound(1, containerWidth / preferredCardWidth, 4);
    m_lastQuickControlColumns = maxColumns;
    const bool singleRowLayout = !items.isEmpty() && items.size() <= maxColumns;

    QLayout *oldLayout = ui->quickControlContainer->layout();
    if (oldLayout)
    {
        QLayoutItem *child = nullptr;
        while ((child = oldLayout->takeAt(0)) != nullptr)
        {
            if (child->widget())
            {
                delete child->widget();
            }
            delete child;
        }

        // 旧网格可能残留行列拉伸信息，直接销毁后重建可避免切换主题后叠压。
        delete oldLayout;
    }

    QGridLayout *gridLayout = new QGridLayout(ui->quickControlContainer);

    gridLayout->setContentsMargins(2, 2, 2, 8);
    gridLayout->setHorizontalSpacing(14);
    gridLayout->setVerticalSpacing(14);
    gridLayout->setAlignment(singleRowLayout ? Qt::AlignCenter : Qt::AlignTop);
    gridLayout->setSizeConstraint(QLayout::SetMinimumSize);

    for (int i = 0; i < maxColumns; ++i)
    {
        gridLayout->setColumnStretch(i, 1);
    }

    int row = 0;
    int col = 0;
    const int maxCols = maxColumns;
    bool selectedSceneExists = false;
    const QPalette palette = this->palette();

    for (const QuickControlDisplayItem &item : items)
    {
        QToolButton *btn = new QToolButton(ui->quickControlContainer);
        btn->setText(localizedQuickControlName(item.displayName, m_languageKey == QStringLiteral("en_US")));
        const QString iconPath = item.iconPath.isEmpty()
                                     ? (item.targetType == "scene" ? QStringLiteral(":/icons/scene.svg")
                                                                   : QStringLiteral(":/icons/devices.svg"))
                                     : item.iconPath;
        const QSize iconSize = singleRowLayout ? QSize(52, 52) : QSize(36, 36);
        btn->setIconSize(iconSize);
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setFixedHeight(singleRowLayout ? 140 : 90);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(false);
        btn->setFocusPolicy(Qt::NoFocus);

        bool selectedStyle = false;
        bool offlineStyle = false;
        bool buttonEnabled = true;

        if (item.targetType == "scene")
        {
            const bool isSelectedScene = (item.targetStringId == m_selectedSceneId);
            if (isSelectedScene)
            {
                selectedSceneExists = true;
            }
            selectedStyle = isSelectedScene;
            btn->setToolTip(isSelectedScene ? QStringLiteral("场景已选中，点击可再次执行")
                                            : QStringLiteral("场景未选中，点击可执行并选中"));
        }
        else
        {
            if (!item.isOnline)
            {
                offlineStyle = true;
                btn->setEnabled(false);
                buttonEnabled = false;
                btn->setToolTip(QStringLiteral("设备离线"));
            }
            else
            {
                selectedStyle = item.isOn;
                btn->setToolTip(item.isOn ? QStringLiteral("当前已开启，点击关闭")
                                          : QStringLiteral("当前已关闭，点击开启"));
            }
        }

        btn->setStyleSheet(quickControlCardStyle(palette, singleRowLayout, offlineStyle, selectedStyle, buttonEnabled));
        btn->setIcon(tintedIcon(iconPath, quickCardIconColor(palette, selectedStyle, offlineStyle, buttonEnabled), iconSize));

        connect(btn, &QToolButton::clicked, this, [=](bool /*checked*/)
                {
            const bool wantToTurnOn = (item.targetType == "device") ? !item.isOn : true;

            QString errorMsg;
            QString warningMsg;
            if (m_quickControlService.executeShortcut(item, wantToTurnOn, &errorMsg, &warningMsg))
            {
                if (item.targetType == "scene")
                {
                    m_selectedSceneId = item.targetStringId;
                }
                loadQuickControls();
                refreshDeviceStatus();
                if (!warningMsg.trimmed().isEmpty())
                {
                    QMessageBox::warning(this, QStringLiteral("部分成功"), warningMsg);
                }
            }
            else
            {
                QMessageBox::warning(this, QStringLiteral("操作失败"), errorMsg);
            } });

        gridLayout->addWidget(btn, row, col);

        ++col;
        if (col >= maxCols)
        {
            col = 0;
            ++row;
        }
    }

    const int rowCount = items.isEmpty() ? 1 : ((items.size() + maxCols - 1) / maxCols);
    const int cardHeight = singleRowLayout ? 140 : 90;
    const int totalHeight = 2 + 8 + rowCount * cardHeight + qMax(0, rowCount - 1) * 14;
    ui->quickControlContainer->setMinimumHeight(0);
    ui->quickControlContainer->setFixedHeight(qMax(100, totalHeight));

    if (!selectedSceneExists)
    {
        m_selectedSceneId.clear();
    }
}

int HomeWidget::quickControlColumnsForWidth(int width) const
{
    const int preferredCardWidth = 220;
    return qBound(1, qMax(1, width) / preferredCardWidth, 4);
}

void HomeWidget::updateEnvironmentData(double temp, double hum)
{
    ui->label_temperature->setText(QString::number(temp, 'f', 1) + QStringLiteral(" °C"));
    ui->label_humidity->setText(QString::number(hum, 'f', 1) + QStringLiteral(" %"));
    applyTemperatureColor(temp);
}

void HomeWidget::updateEnvironmentUI(const QJsonObject &data)
{
    if (data.contains(QStringLiteral("temperature")))
    {
        const double temperature = data.value(QStringLiteral("temperature")).toDouble();
        ui->label_temperature->setText(QString::number(temperature, 'f', 1) + QStringLiteral(" °C"));
        applyTemperatureColor(temperature);
    }

    if (data.contains(QStringLiteral("humidity")))
    {
        ui->label_humidity->setText(QString::number(data.value(QStringLiteral("humidity")).toDouble(), 'f', 1) + QStringLiteral(" %"));
    }
}

void HomeWidget::refreshQuickControls()
{
    loadQuickControls();
}

void HomeWidget::onEnvironmentSnapshotLoaded(HomeEnvironmentRefreshResult result)
{
    if (!result.errorText.trimmed().isEmpty())
    {
        qWarning() << "读取环境快照或评估环境报警失败:" << result.errorText;
        return;
    }

    if (result.snapshot.has_value())
    {
        const double nextTemperature = result.snapshot->temperature;
        const bool temperatureChanged = m_hasLastTemperature &&
                                        !qFuzzyCompare(m_lastTemperature + 1.0, nextTemperature + 1.0);
        updateEnvironmentData(result.snapshot->temperature, result.snapshot->humidity);

        if (temperatureChanged)
        {
            QString errorText;
            const QList<QJsonObject> triggeredAlarms =
                m_alarmService.evaluateEnvironmentSnapshot(result.snapshot.value(), &errorText);
            if (!errorText.trimmed().isEmpty())
            {
                qWarning() << "首页温度变化后立即报警评估失败:" << errorText;
            }
            else
            {
                for (const QJsonObject &alarmData : triggeredAlarms)
                {
                    emit alarmTriggered(alarmData);
                }
            }
        }

        m_lastTemperature = nextTemperature;
        m_hasLastTemperature = true;
    }

    updateDeviceStatusLabel(result.deviceStatus);
}

void HomeWidget::applyTemperatureColor(double temperature)
{
    const QString color = m_environmentService.temperatureColor(temperature);
    ui->label_temperature->setStyleSheet(QStringLiteral("color: %1; font-size: 24pt; font-weight: 700;").arg(color));
}

void HomeWidget::refreshDeviceStatus()
{
    m_environmentService.refreshNow();
}

void HomeWidget::updateDeviceStatusLabel(const DeviceStatusSummary &summary)
{
    if (m_languageKey == QStringLiteral("en_US"))
    {
        ui->label_deviceCount->setText(QStringLiteral("Online: %1/%2").arg(summary.onlineCount).arg(summary.totalCount));
        return;
    }
    ui->label_deviceCount->setText(QStringLiteral("在线: %1/%2").arg(summary.onlineCount).arg(summary.totalCount));
}

void HomeWidget::refreshStaticTexts()
{
    const bool isEnglish = (m_languageKey == QStringLiteral("en_US"));
    ui->label_title->setText(isEnglish ? QStringLiteral("Smart Home Control Center") : QStringLiteral("智能家居控制中心"));
    ui->groupBox_temp->setTitle(isEnglish ? QStringLiteral("Temperature") : QStringLiteral("当前温度"));
    ui->groupBox_humidity->setTitle(isEnglish ? QStringLiteral("Humidity") : QStringLiteral("当前湿度"));
    ui->groupBox_devices->setTitle(isEnglish ? QStringLiteral("Device Status") : QStringLiteral("设备状态"));
    ui->groupBox_quickControl->setTitle(isEnglish ? QStringLiteral("Quick Controls") : QStringLiteral("快捷控制"));
    ui->groupBox_alarm->setTitle(isEnglish ? QStringLiteral("Recent Alarms") : QStringLiteral("最近报警"));
    if (ui->textEdit_alarmLog->toPlainText().trimmed().isEmpty() || ui->textEdit_alarmLog->toPlainText().contains(QStringLiteral("暂无报警")) || ui->textEdit_alarmLog->toPlainText().contains(QStringLiteral("No alarms")))
    {
        ui->textEdit_alarmLog->setHtml(isEnglish
                                           ? QStringLiteral("<!DOCTYPE HTML><html><body><p style=\"color:green;\">System running normally, no alarms.</p></body></html>")
                                           : QStringLiteral("<!DOCTYPE HTML><html><body><p style=\"color:green;\">系统运行正常，暂无报警信息</p></body></html>"));
    }
    ensureQuickControlEditButton();
}

void HomeWidget::scheduleThemeRefresh()
{
    if (!ui || m_themeRefreshScheduled)
    {
        return;
    }

    m_themeRefreshScheduled = true;
    QMetaObject::invokeMethod(this, [this]()
                              {
        m_themeRefreshScheduled = false;
        if (!ui)
        {
            return;
        }

        ensureQuickControlEditButton();
        refreshQuickControls();
        refreshDeviceStatus(); }, Qt::QueuedConnection);
}

void HomeWidget::changeEvent(QEvent *event)
{
    QWidget::changeEvent(event);

    if (!ui)
    {
        return;
    }

    if (event->type() == QEvent::PaletteChange ||
        event->type() == QEvent::ApplicationPaletteChange ||
        event->type() == QEvent::StyleChange)
    {
        scheduleThemeRefresh();
    }
}

void HomeWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    m_environmentService.startPolling(500);
    refreshQuickControls();
    refreshDeviceStatus();
}

void HomeWidget::hideEvent(QHideEvent *event)
{
    QWidget::hideEvent(event);
    m_environmentService.stopPolling();
}

void HomeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (event->size().width() != event->oldSize().width())
    {
        const int newColumns = quickControlColumnsForWidth(ui->quickControlContainer ? ui->quickControlContainer->width() : event->size().width());
        if (newColumns != m_lastQuickControlColumns && m_quickControlResizeTimer)
        {
            m_quickControlResizeTimer->start();
        }
    }
}
