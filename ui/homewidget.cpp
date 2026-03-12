#include "homewidget.h"
#include "quickcontrolmanagedialog.h"
#include "ui_homewidget.h"

#include <QtConcurrent>
#include <QDebug>
#include <QColor>
#include <QGridLayout>
#include <QHash>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
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

QIcon tintedIcon(const QString &path, const QColor &color)
{
    const QIcon baseIcon(path);
    const QPixmap source = baseIcon.pixmap(QSize(40, 40));
    if (source.isNull())
    {
        return baseIcon;
    }

    QPixmap tinted(source.size());
    tinted.fill(Qt::transparent);
    QPainter painter(&tinted);
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(tinted.rect(), color);
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

    QColor background = dark ? mixColor(base, QColor("#FFFFFF"), 0.12) : QColor("#F7F9FC");
    QColor border = dark ? mixColor(base, QColor("#FFFFFF"), 0.22) : QColor("#D7E3F1");
    QColor hover = dark ? mixColor(background, QColor("#FFFFFF"), 0.08) : mixColor(background, QColor("#000000"), 0.04);
    QColor textColor = dark ? mixColor(text, QColor("#FFFFFF"), 0.12) : QColor("#5E6A78");

    if (isSelected)
    {
        background = dark ? mixColor(accent, base, 0.78) : mixColor(accent, QColor("#FFFFFF"), 0.90);
        border = dark ? mixColor(accent, QColor("#FFFFFF"), 0.20) : mixColor(accent, QColor("#000000"), 0.08);
        hover = dark ? mixColor(background, QColor("#FFFFFF"), 0.08) : mixColor(background, QColor("#000000"), 0.04);
        textColor = dark ? mixColor(accent, QColor("#FFFFFF"), 0.24) : QColor("#1565C0");
    }
    else if (isOffline || !isEnabled)
    {
        background = dark ? mixColor(base, QColor("#FFFFFF"), 0.06) : QColor("#F3F4F6");
        border = dark ? mixColor(base, QColor("#FFFFFF"), 0.14) : QColor("#D8DCE2");
        hover = background;
        textColor = dark ? mixColor(text, base, 0.35) : QColor("#8F99A4");
    }

    const QString paddingStyle = singleRowLayout ? QStringLiteral("padding: 16px 8px 12px 8px;") : QStringLiteral("padding: 14px 8px 10px 8px;");
    return QStringLiteral("QToolButton { background: %1; border: 1px solid %2; border-radius: 12px; color: %3; font-weight: 600; %4 }"
                          "QToolButton:hover { background: %5; }")
        .arg(cssColor(background), cssColor(border), cssColor(textColor), paddingStyle, cssColor(hover));
}

QColor quickCardIconColor(const QPalette &palette, bool isSelected, bool isOffline, bool isEnabled)
{
    const bool dark = isDarkTheme(palette);
    const QColor text = palette.color(QPalette::WindowText);
    const QColor accent = palette.color(QPalette::Highlight);
    if (isSelected)
    {
        return dark ? mixColor(accent, QColor("#FFFFFF"), 0.28) : QColor("#1565C0");
    }
    if (isOffline || !isEnabled)
    {
        return dark ? mixColor(text, QColor("#111827"), 0.55) : QColor("#9098A2");
    }
    return dark ? mixColor(text, QColor("#FFFFFF"), 0.18) : QColor("#2F3D4D");
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
    : QWidget(parent), ui(new Ui::HomeWidget), m_environmentRefreshTimer(new QTimer(this)), m_environmentWatcher(new QFutureWatcher<HomeEnvironmentRefreshResult>(this)), m_deviceStatusWatcher(new QFutureWatcher<DeviceStatusSummary>(this))
{
    ui->setupUi(this);
    ui->label_title->setStyleSheet(QStringLiteral("font-size: 18pt; font-weight: 700;"));
    ui->label_temperature->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight: 700;"));
    ui->label_humidity->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight: 700;"));
    ui->label_deviceCount->setStyleSheet(QStringLiteral("font-size: 16pt; font-weight: 600;"));
    ensureQuickControlEditButton();
    applyLanguage(QStringLiteral("zh_CN"));

    connect(m_environmentWatcher, &QFutureWatcher<HomeEnvironmentRefreshResult>::finished, this, &HomeWidget::onEnvironmentSnapshotLoaded);
    connect(m_deviceStatusWatcher, &QFutureWatcher<DeviceStatusSummary>::finished, this, &HomeWidget::onDeviceStatusLoaded);
    connect(m_environmentRefreshTimer, &QTimer::timeout, this, [this]()
            {
        refreshEnvironmentSnapshot();
        refreshDeviceStatus();
        refreshQuickControls(); });

    refreshDeviceStatus();
    refreshEnvironmentSnapshot();
    loadQuickControls();
    m_environmentRefreshTimer->start(3000);
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

    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(14);
    gridLayout->setVerticalSpacing(14);
    gridLayout->setAlignment(singleRowLayout ? Qt::AlignCenter : Qt::AlignTop);

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
        btn->setIconSize(singleRowLayout ? QSize(40, 40) : QSize(36, 36));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        btn->setFixedHeight(singleRowLayout ? 138 : 130);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(false);

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
        btn->setIcon(tintedIcon(iconPath, quickCardIconColor(palette, selectedStyle, offlineStyle, buttonEnabled)));

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
        gridLayout->setRowMinimumHeight(row, singleRowLayout ? 138 : 130);

        ++col;
        if (col >= maxCols)
        {
            col = 0;
            ++row;
        }
    }

    if (!selectedSceneExists)
    {
        m_selectedSceneId.clear();
    }
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

void HomeWidget::refreshEnvironmentSnapshot()
{
    if (m_environmentWatcher->isRunning())
    {
        return;
    }

    m_environmentWatcher->setProperty("requestId", ++m_environmentRequestId);
    m_environmentWatcher->setFuture(QtConcurrent::run([]()
                                                      {
        HomeEnvironmentRefreshResult result;

        EnvRecordDao envRecordDao;
        const std::optional<EnvRealtimeSnapshot> snapshot = envRecordDao.getLatestRealtimeSnapshot();
        if (!snapshot.has_value())
        {
            result.success = envRecordDao.lastErrorText().trimmed().isEmpty();
            result.errorText = envRecordDao.lastErrorText();
            return result;
        }

        result.snapshot = snapshot;

        AlarmService alarmService;
        QString errorText;
        result.triggeredAlarms = alarmService.evaluateEnvironmentSnapshot(snapshot.value(), &errorText);
        if (!errorText.isEmpty())
        {
            result.errorText = errorText;
            return result;
        }

        result.success = true;
        return result; }));
}

void HomeWidget::refreshDeviceStatusAsync()
{
    if (m_deviceStatusWatcher->isRunning())
    {
        return;
    }

    m_deviceStatusWatcher->setProperty("requestId", ++m_deviceStatusRequestId);
    m_deviceStatusWatcher->setFuture(QtConcurrent::run([]()
                                                       {
        SettingsService settingsService;
        return settingsService.loadDeviceStatusSummary(); }));
}

void HomeWidget::onEnvironmentSnapshotLoaded()
{
    if (m_environmentWatcher->property("requestId").toInt() != m_environmentRequestId)
    {
        return;
    }

    const HomeEnvironmentRefreshResult result = m_environmentWatcher->result();
    if (!result.errorText.trimmed().isEmpty())
    {
        qWarning() << "读取环境快照或评估环境报警失败:" << result.errorText;
        return;
    }

    if (result.snapshot.has_value())
    {
        updateEnvironmentData(result.snapshot->temperature, result.snapshot->humidity);
    }

    for (const QJsonObject &alarmData : result.triggeredAlarms)
    {
        emit alarmTriggered(alarmData);
    }
}

void HomeWidget::onDeviceStatusLoaded()
{
    if (m_deviceStatusWatcher->property("requestId").toInt() != m_deviceStatusRequestId)
    {
        return;
    }

    updateDeviceStatusLabel(m_deviceStatusWatcher->result());
}

void HomeWidget::on_btnGoHome_clicked()
{
    qDebug() << "返回首页按钮被点击";
}

void HomeWidget::applyTemperatureColor(double temperature)
{
    const QString color = m_environmentService.temperatureColor(temperature);
    ui->label_temperature->setStyleSheet(QStringLiteral("color: %1; font-size: 24pt; font-weight: 700;").arg(color));
}

void HomeWidget::refreshDeviceStatus()
{
    refreshDeviceStatusAsync();
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
    if (ui->textEdit_alarmLog->toPlainText().trimmed().isEmpty()
        || ui->textEdit_alarmLog->toPlainText().contains(QStringLiteral("暂无报警"))
        || ui->textEdit_alarmLog->toPlainText().contains(QStringLiteral("No alarms")))
    {
        ui->textEdit_alarmLog->setHtml(isEnglish
                                           ? QStringLiteral("<!DOCTYPE HTML><html><body><p style=\"color:green;\">System running normally, no alarms.</p></body></html>")
                                           : QStringLiteral("<!DOCTYPE HTML><html><body><p style=\"color:green;\">系统运行正常，暂无报警信息</p></body></html>"));
    }
    ensureQuickControlEditButton();
}

void HomeWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshEnvironmentSnapshot();
    refreshDeviceStatus();
    refreshQuickControls();
}

void HomeWidget::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    if (event->size().width() != event->oldSize().width())
    {
        refreshQuickControls();
    }
}
