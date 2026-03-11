#include "homewidget.h"
#include "quickcontrolmanagedialog.h"
#include "ui_homewidget.h"

#include <QtConcurrent>
#include <QDebug>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>

namespace
{
    QString quickControlCardStyle(const char *baseStyle, bool singleRowLayout)
    {
        const QString layoutStyle = singleRowLayout
                                        ? QStringLiteral("QToolButton { padding: 18px 8px 14px 8px; }")
                                        : QStringLiteral("QToolButton { padding: 18px 8px 12px 8px; }");
        return QString::fromLatin1(baseStyle) + layoutStyle;
    }

    const char *kDeviceOnStyle =
        "QToolButton { background-color: #E3F2FD; border: 1px solid #2196F3; border-radius: 10px; color: #1976D2; font-weight: bold; padding: 8px; }"
        "QToolButton:hover { background-color: #BBDEFB; }";

    const char *kDeviceOffStyle =
        "QToolButton { background-color: #F7F7F7; border: 1px solid #E1E1E1; border-radius: 10px; color: #727272; padding: 8px; }"
        "QToolButton:hover { background-color: #EEEEEE; }";

    const char *kDeviceOfflineStyle =
        "QToolButton { background-color: #F2F2F2; border: 1px dashed #C9C9C9; border-radius: 10px; color: #9A9A9A; padding: 8px; }";

    const char *kSceneSelectedStyle =
        "QToolButton { background-color: #D7F4E6; border: 1px solid #2EAD75; border-radius: 10px; color: #1B7E52; font-weight: bold; padding: 8px; }"
        "QToolButton:hover { background-color: #C6ECD9; }";

    const char *kSceneUnselectedStyle =
        "QToolButton { background-color: #F5F5F5; border: 1px solid #DEDEDE; border-radius: 10px; color: #8A8A8A; padding: 8px; }"
        "QToolButton:hover { background-color: #EDEDED; }";
}

HomeWidget::HomeWidget(QWidget *parent)
    : QWidget(parent), ui(new Ui::HomeWidget), m_environmentRefreshTimer(new QTimer(this)), m_environmentWatcher(new QFutureWatcher<HomeEnvironmentRefreshResult>(this)), m_deviceStatusWatcher(new QFutureWatcher<DeviceStatusSummary>(this))
{
    ui->setupUi(this);
    ensureQuickControlEditButton();

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

    m_editQuickControlButton->setText(QStringLiteral("编辑快捷控制"));
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
    const int maxColumns = 4;
    const bool singleRowLayout = !items.isEmpty() && items.size() <= maxColumns;

    QLayout *oldLayout = ui->quickControlContainer->layout();
    if (oldLayout)
    {
        QLayoutItem *child = nullptr;
        while ((child = oldLayout->takeAt(0)) != nullptr)
        {
            if (child->widget())
            {
                child->widget()->deleteLater();
            }
            delete child;
        }
    }
    else
    {
        oldLayout = new QGridLayout(ui->quickControlContainer);
        oldLayout->setContentsMargins(0, 0, 0, 0);
        oldLayout->setSpacing(10);
    }

    QGridLayout *gridLayout = qobject_cast<QGridLayout *>(oldLayout);
    if (!gridLayout)
    {
        return;
    }

    gridLayout->setContentsMargins(0, 0, 0, 0);
    gridLayout->setHorizontalSpacing(14);
    gridLayout->setVerticalSpacing(14);
    gridLayout->setAlignment(singleRowLayout ? Qt::AlignCenter : Qt::AlignTop);

    int row = 0;
    int col = 0;
    const int maxCols = 4;
    bool selectedSceneExists = false;

    for (const QuickControlDisplayItem &item : items)
    {
        QToolButton *btn = new QToolButton(ui->quickControlContainer);
        btn->setText(item.displayName);
        const QString iconPath = item.iconPath.isEmpty()
                                     ? (item.targetType == "scene" ? QStringLiteral(":/icons/scene.svg")
                                                                   : QStringLiteral(":/icons/devices.svg"))
                                     : item.iconPath;
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(singleRowLayout ? QSize(40, 40) : QSize(36, 36));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setSizePolicy(QSizePolicy::Expanding, singleRowLayout ? QSizePolicy::Fixed : QSizePolicy::Expanding);
        btn->setMinimumHeight(150);
        if (singleRowLayout)
        {
            btn->setMaximumHeight(150);
        }
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(false);

        if (item.targetType == "scene")
        {
            const bool isSelectedScene = (item.targetStringId == m_selectedSceneId);
            if (isSelectedScene)
            {
                selectedSceneExists = true;
            }
            btn->setStyleSheet(quickControlCardStyle(isSelectedScene ? kSceneSelectedStyle : kSceneUnselectedStyle, singleRowLayout));
            btn->setToolTip(isSelectedScene ? QStringLiteral("场景已选中，点击可再次执行")
                                            : QStringLiteral("场景未选中，点击可执行并选中"));
        }
        else
        {
            if (!item.isOnline)
            {
                btn->setStyleSheet(quickControlCardStyle(kDeviceOfflineStyle, singleRowLayout));
                btn->setEnabled(false);
                btn->setToolTip(QStringLiteral("设备离线"));
            }
            else
            {
                btn->setStyleSheet(quickControlCardStyle(item.isOn ? kDeviceOnStyle : kDeviceOffStyle, singleRowLayout));
                btn->setToolTip(item.isOn ? QStringLiteral("当前已开启，点击关闭")
                                          : QStringLiteral("当前已关闭，点击开启"));
            }
        }

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
    ui->label_temperature->setStyleSheet(QStringLiteral("color: %1; font-weight: bold;").arg(color));
}

void HomeWidget::refreshDeviceStatus()
{
    refreshDeviceStatusAsync();
}

void HomeWidget::updateDeviceStatusLabel(const DeviceStatusSummary &summary)
{
    ui->label_deviceCount->setText(QStringLiteral("在线: %1/%2").arg(summary.onlineCount).arg(summary.totalCount));
}

void HomeWidget::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    refreshEnvironmentSnapshot();
    refreshDeviceStatus();
    refreshQuickControls();
}
