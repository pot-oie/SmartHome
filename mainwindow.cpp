#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/homewidget.h"
#include "ui/devicecontrolwidget.h"
#include "ui/scenewidget.h"
#include "ui/historywidget.h"
#include "ui/alarmwidget.h"
#include "ui/settingswidget.h"

#include <QApplication>
#include <QFile>

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
    HomeWidget *homeWidget = new HomeWidget(this);
    ui->stackWidget->addWidget(homeWidget);                    // 索引 0
    ui->stackWidget->addWidget(new DeviceControlWidget(this)); // 索引 1
    ui->stackWidget->addWidget(new SceneWidget(this));         // 索引 2
    ui->stackWidget->addWidget(new HistoryWidget(this));       // 索引 3
    ui->stackWidget->addWidget(new AlarmWidget(this));         // 索引 4
    SettingsWidget *settingsWidget = new SettingsWidget(this);
    ui->stackWidget->addWidget(settingsWidget); // 索引 5

    // 默认显示首页
    ui->stackWidget->setCurrentIndex(0);
    ui->navBar->setCurrentRow(0);

    // 连接导航栏点击信号
    connect(ui->navBar, &QListWidget::currentRowChanged, this, &MainWindow::onNavBarItemClicked);
    connect(settingsWidget, &SettingsWidget::themeChanged, this, &MainWindow::onThemeChanged);
    connect(settingsWidget, &SettingsWidget::languageChanged, this, &MainWindow::onLanguageChanged);
    connect(settingsWidget, &SettingsWidget::devicesChanged, homeWidget, &HomeWidget::refreshDeviceStatus);
}

void MainWindow::onNavBarItemClicked(int index)
{
    // 切换到对应的页面
    ui->stackWidget->setCurrentIndex(index);

    // 页面切换时主动刷新，避免快捷控制和设备控制状态显示不一致
    if (index == 0 && m_homeWidget)
    {
        m_homeWidget->refreshQuickControls();
    }
    else if (index == 1 && m_deviceControlWidget)
    {
        m_deviceControlWidget->refreshDevices();
    }
}

void MainWindow::onThemeChanged(const QString &themeName)
{
    applyTheme(themeName);
}

void MainWindow::onLanguageChanged(const QString &languageKey)
{
    applyLanguage(languageKey);
}

void MainWindow::applyTheme(const QString &themeName)
{
    QString styleSheet;
    if (themeName == "dark")
    {
        styleSheet = loadStyleSheet(":/style_dark.qss");
    }
    else
    {
        styleSheet = loadStyleSheet(":/style.qss");
    }

    if (!styleSheet.isEmpty())
    {
        qApp->setStyleSheet(styleSheet);
    }
}

void MainWindow::applyLanguage(const QString &languageKey)
{
    qApp->removeTranslator(&m_translator);

    if (languageKey == "en_US")
    {
        if (m_translator.load(":/i18n/SmartHome_en_US"))
        {
            qApp->installTranslator(&m_translator);
        }
    }

    // 最低成本刷新：重设窗口标题和导航栏文本
    setWindowTitle(tr("智能家居监控平台"));
    const QStringList navTexts = {tr("首页"), tr("设备控制"), tr("场景管理"), tr("历史记录"), tr("报警设置"), tr("系统设置")};
    for (int i = 0; i < ui->navBar->count() && i < navTexts.size(); ++i)
    {
        ui->navBar->item(i)->setText(navTexts.at(i));
    }
}

QString MainWindow::loadStyleSheet(const QString &resourcePath) const
{
    QFile file(resourcePath);
    if (!file.open(QFile::ReadOnly))
    {
        return QString();
    }
    return QString::fromUtf8(file.readAll());
}
