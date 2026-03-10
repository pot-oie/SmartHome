#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/homewidget.h"
#include "ui/devicecontrolwidget.h"
#include "ui/scenewidget.h"
#include "ui/historywidget.h"
#include "ui/alarmwidget.h"
#include "ui/settingswidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initUI();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initUI()
{
    // 设置窗口标题和大小
    setWindowTitle("智能家居监控平台");
    resize(1024, 768);

    // 设置导航栏图标大小
    ui->navBar->setIconSize(QSize(24, 24));

    // 初始化导航栏（带图标）
    auto addNavItem = [this](const QString &icon, const QString &text)
    {
        QIcon navIcon(icon);
        QListWidgetItem *item = new QListWidgetItem(navIcon, text);
        ui->navBar->addItem(item);
    };

    addNavItem(":/icons/home.svg", "首页");
    addNavItem(":/icons/devices.svg", "设备控制");
    addNavItem(":/icons/scene.svg", "场景管理");
    addNavItem(":/icons/history.svg", "历史记录");
    addNavItem(":/icons/alarm.svg", "报警设置");
    addNavItem(":/icons/settings.svg", "系统设置");

    // 创建各个子页面并添加到堆栈窗口
    ui->stackWidget->addWidget(new HomeWidget(this));          // 索引 0
    ui->stackWidget->addWidget(new DeviceControlWidget(this)); // 索引 1
    ui->stackWidget->addWidget(new SceneWidget(this));         // 索引 2
    ui->stackWidget->addWidget(new HistoryWidget(this));       // 索引 3
    ui->stackWidget->addWidget(new AlarmWidget(this));         // 索引 4
    ui->stackWidget->addWidget(new SettingsWidget(this));      // 索引 5

    // 默认显示首页
    ui->stackWidget->setCurrentIndex(0);
    ui->navBar->setCurrentRow(0);

    // 连接导航栏点击信号
    connect(ui->navBar, &QListWidget::currentRowChanged, this, &MainWindow::onNavBarItemClicked);
}

void MainWindow::onNavBarItemClicked(int index)
{
    // 切换到对应的页面
    ui->stackWidget->setCurrentIndex(index);
}
