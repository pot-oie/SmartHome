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

    // 初始化导航栏
    ui->navBar->addItem("🏠 首页");
    ui->navBar->addItem("🔧 设备控制");
    ui->navBar->addItem("🎬 场景管理");
    ui->navBar->addItem("📊 历史记录");
    ui->navBar->addItem("🚨 报警设置");
    ui->navBar->addItem("⚙️ 系统设置");

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
