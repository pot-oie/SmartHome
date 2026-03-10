#include "homewidget.h"
#include "ui_homewidget.h"
#include "quickcontrolmanagedialog.h"
#include <QDebug>
#include <QTimer>
#include <QGridLayout>
#include <QToolButton>
#include <QMessageBox>
#include <QPushButton>

namespace
{
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

HomeWidget::HomeWidget(QWidget *parent) : QWidget(parent),
                                          ui(new Ui::HomeWidget)
{
    ui->setupUi(this);
    refreshDeviceStatus();

    const EnvironmentSnapshot initial = m_environmentService.generateInitialSnapshot();
    updateEnvironmentData(initial.temperature, initial.humidity);

    // 初始化时加载快捷控制面板
    loadQuickControls();

    // 使用定时器模拟数据更新（每5秒更新一次）
    QTimer *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this]()
            {
        const EnvironmentSnapshot snapshot = m_environmentService.generateNextSnapshot();
        updateEnvironmentData(snapshot.temperature, snapshot.humidity);
        refreshDeviceStatus(); });
    timer->start(5000);
}

HomeWidget::~HomeWidget()
{
    delete ui;
}

void HomeWidget::initConnections()
{
    // 连接已经在 cpp 对应的位置建立
}

void HomeWidget::onQuickControlClicked()
{
    qDebug() << "快捷控制被触发";
}

// 动态渲染快捷控制面板
void HomeWidget::loadQuickControls()
{
    // 1. 获取后端组装好的快捷项数据
    QList<QuickControlDisplayItem> items = m_quickControlService.getHomeShortcuts();

    // 2. 获取或创建容器的布局管理器
    QLayout *oldLayout = ui->quickControlContainer->layout();
    if (oldLayout)
    {
        // 如果已经有布局，清空里面的旧按钮（用于状态刷新）
        QLayoutItem *child;
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
        // 如果没有布局，创建一个网格布局
        oldLayout = new QGridLayout(ui->quickControlContainer);
        oldLayout->setContentsMargins(0, 0, 0, 0);
        oldLayout->setSpacing(10); // 按钮之间的间距
    }

    QGridLayout *gridLayout = qobject_cast<QGridLayout *>(oldLayout);

    // 3. 动态生成按钮并放入网格
    int row = 0;
    int col = 0;
    const int maxCols = 4; // 每行最多显示4个，你可以根据UI宽度自适应调整
    bool selectedSceneExists = false;

    for (const auto &item : items)
    {
        // 使用 QToolButton 因为它非常适合“上图标+下文字”的展现形式
        QToolButton *btn = new QToolButton(ui->quickControlContainer);
        btn->setText(item.displayName);
        const QString iconPath = item.iconPath.isEmpty()
                                     ? (item.targetType == "scene" ? QString(":/icons/scene.svg") : QString(":/icons/devices.svg"))
                                     : item.iconPath;
        btn->setIcon(QIcon(iconPath));
        btn->setIconSize(QSize(36, 36));
        btn->setToolButtonStyle(Qt::ToolButtonTextUnderIcon);
        btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        btn->setMinimumHeight(80);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRaise(false);

        // 4. 根据设备/场景类型应用不同风格
        if (item.targetType == "scene")
        {
            const bool isSelectedScene = (item.targetStringId == m_selectedSceneId);
            if (isSelectedScene)
            {
                selectedSceneExists = true;
            }
            btn->setStyleSheet(isSelectedScene ? kSceneSelectedStyle : kSceneUnselectedStyle);
            btn->setToolTip(isSelectedScene ? QStringLiteral("场景已选中，点击可再次执行")
                                            : QStringLiteral("场景未选中，点击可执行并选中"));
        }
        else
        {
            if (!item.isOnline)
            {
                btn->setStyleSheet(kDeviceOfflineStyle);
                btn->setEnabled(false);
                btn->setToolTip(QStringLiteral("设备离线"));
            }
            else
            {
                btn->setStyleSheet(item.isOn ? kDeviceOnStyle : kDeviceOffStyle);
                btn->setToolTip(item.isOn ? QStringLiteral("当前已开启，点击关闭")
                                          : QStringLiteral("当前已关闭，点击开启"));
            }
        }

        // 5. 使用 Lambda 表达式捕获 item 并绑定点击事件
        connect(btn, &QToolButton::clicked, this, [=](bool /*checked*/)
                {
            // 设备走开关切换；场景固定为触发动作
            bool wantToTurnOn = (item.targetType == "device") ? !item.isOn : true;

            QString errorMsg;
            // 直接调用 Service 统一执行
            if (m_quickControlService.executeShortcut(item, wantToTurnOn, &errorMsg)) {
                if (item.targetType == "scene") {
                    m_selectedSceneId = item.targetStringId;
                }
                // 执行成功后，重新读取数据库并渲染，刷新 UI 状态
                this->loadQuickControls();
            } else {
                QMessageBox::warning(this, "操作失败", errorMsg);
            } });

        // 加入网格布局
        gridLayout->addWidget(btn, row, col);

        // 控制换行逻辑
        col++;
        if (col >= maxCols)
        {
            col = 0;
            row++;
        }
    }

    // 选中的场景如果已被移除，自动清空选中态，避免错误高亮
    if (!selectedSceneExists)
    {
        m_selectedSceneId.clear();
    }
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

void HomeWidget::refreshQuickControls()
{
    loadQuickControls();
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

void HomeWidget::refreshDeviceStatus()
{
    const DeviceStatusSummary summary = m_settingsService.loadDeviceStatusSummary();
    updateDeviceStatusLabel(summary);
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
