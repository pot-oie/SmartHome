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
#include <QListWidgetItem>

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
    setWindowTitle(QStringLiteral("智能家居监控平台"));
    resize(1024, 768);

    ui->navBar->setIconSize(QSize(24, 24));

    auto addNavItem = [this](const QString &icon, const QString &text)
    {
        QListWidgetItem *item = new QListWidgetItem(QIcon(icon), text);
        ui->navBar->addItem(item);
    };

    addNavItem(QStringLiteral(":/icons/home.svg"), QStringLiteral("首页"));
    addNavItem(QStringLiteral(":/icons/devices.svg"), QStringLiteral("设备控制"));
    addNavItem(QStringLiteral(":/icons/scene.svg"), QStringLiteral("场景管理"));
    addNavItem(QStringLiteral(":/icons/history.svg"), QStringLiteral("历史记录"));
    addNavItem(QStringLiteral(":/icons/alarm.svg"), QStringLiteral("报警设置"));
    addNavItem(QStringLiteral(":/icons/settings.svg"), QStringLiteral("系统设置"));

    m_homeWidget = new HomeWidget(this);
    ui->stackWidget->addWidget(m_homeWidget);

    m_deviceControlWidget = new DeviceControlWidget(this);
    ui->stackWidget->addWidget(m_deviceControlWidget);

    m_sceneWidget = new SceneWidget(this);
    ui->stackWidget->addWidget(m_sceneWidget);

    m_historyWidget = new HistoryWidget(this);
    ui->stackWidget->addWidget(m_historyWidget);

    m_alarmWidget = new AlarmWidget(this);
    ui->stackWidget->addWidget(m_alarmWidget);

    SettingsWidget *settingsWidget = new SettingsWidget(this);
    ui->stackWidget->addWidget(settingsWidget);

    ui->stackWidget->setCurrentIndex(0);
    ui->navBar->setCurrentRow(0);

    connect(ui->navBar, &QListWidget::currentRowChanged, this, &MainWindow::onNavBarItemClicked);
    connect(settingsWidget, &SettingsWidget::themeChanged, this, &MainWindow::onThemeChanged);
    connect(settingsWidget, &SettingsWidget::languageChanged, this, &MainWindow::onLanguageChanged);
    connect(settingsWidget, &SettingsWidget::devicesChanged, m_homeWidget, &HomeWidget::refreshDeviceStatus);
    connect(settingsWidget, &SettingsWidget::devicesChanged, m_homeWidget, &HomeWidget::refreshQuickControls);
    connect(settingsWidget, &SettingsWidget::devicesChanged, m_deviceControlWidget, &DeviceControlWidget::refreshDevices);
    connect(settingsWidget, &SettingsWidget::devicesChanged, m_historyWidget, &HistoryWidget::refreshData);
    connect(m_homeWidget, &HomeWidget::alarmTriggered, m_alarmWidget, &AlarmWidget::triggerAlarm);
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_homeWidget, &HomeWidget::refreshQuickControls);
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_homeWidget, &HomeWidget::refreshDeviceStatus);
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_deviceControlWidget, &DeviceControlWidget::refreshDevices);
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_historyWidget, &HistoryWidget::refreshData);
}

void MainWindow::onNavBarItemClicked(int index)
{
    ui->stackWidget->setCurrentIndex(index);

    if (index == 0 && m_homeWidget)
    {
        m_homeWidget->refreshQuickControls();
        m_homeWidget->refreshDeviceStatus();
    }
    else if (index == 1 && m_deviceControlWidget)
    {
        m_deviceControlWidget->refreshDevices();
    }
    else if (index == 2 && m_sceneWidget)
    {
        m_sceneWidget->refreshScenes();
    }
    else if (index == 3 && m_historyWidget)
    {
        m_historyWidget->refreshData();
    }
    else if (index == 4 && m_alarmWidget)
    {
        m_alarmWidget->refreshData();
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
    if (themeName == QStringLiteral("dark"))
    {
        styleSheet = loadStyleSheet(QStringLiteral(":/style_dark.qss"));
    }
    else
    {
        styleSheet = loadStyleSheet(QStringLiteral(":/style.qss"));
    }

    if (!styleSheet.isEmpty())
    {
        qApp->setStyleSheet(styleSheet);
    }
}

void MainWindow::applyLanguage(const QString &languageKey)
{
    qApp->removeTranslator(&m_translator);

    if (languageKey == QStringLiteral("en_US"))
    {
        if (m_translator.load(QStringLiteral(":/i18n/SmartHome_en_US")))
        {
            qApp->installTranslator(&m_translator);
        }
    }

    setWindowTitle(tr("智能家居监控平台"));
    const QStringList navTexts = {
        tr("首页"),
        tr("设备控制"),
        tr("场景管理"),
        tr("历史记录"),
        tr("报警设置"),
        tr("系统设置")};
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
        return {};
    }
    return QString::fromUtf8(file.readAll());
}
