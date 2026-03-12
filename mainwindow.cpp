#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui/homewidget.h"
#include "ui/devicecontrolwidget.h"
#include "ui/scenewidget.h"
#include "ui/historywidget.h"
#include "ui/alarmwidget.h"
#include "ui/settingswidget.h"

#include <QApplication>
#include <QColor>
#include <QFile>
#include <QListWidgetItem>
#include <QPainter>
#include <QPixmap>
#include <QStyle>

namespace
{
    QIcon tintedIcon(const QString &path, const QColor &color)
    {
        const QIcon baseIcon(path);
        const QSize iconSize(24, 24);
        const QPixmap src = baseIcon.pixmap(iconSize);
        if (src.isNull())
        {
            return baseIcon;
        }

        QPixmap tinted(src.size());
        tinted.fill(Qt::transparent);
        QPainter painter(&tinted);
        painter.drawPixmap(0, 0, src);
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(tinted.rect(), color);
        painter.end();
        return QIcon(tinted);
    }
}

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
    ui->navBar->setSpacing(2);
    ui->navBar->setFocusPolicy(Qt::NoFocus);

    auto addNavItem = [this](const QString &icon, const QString &text)
    {
        QListWidgetItem *item = new QListWidgetItem(QIcon(icon), text);
        item->setData(Qt::UserRole, icon);
        item->setSizeHint(QSize(160, 44));
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
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

    m_settingsWidget = new SettingsWidget(this);
    ui->stackWidget->addWidget(m_settingsWidget);

    ui->stackWidget->setCurrentIndex(0);
    ui->navBar->setCurrentRow(0);

    connect(ui->navBar, &QListWidget::currentRowChanged, this, &MainWindow::onNavBarItemClicked);
    connect(m_settingsWidget, &SettingsWidget::themeChanged, this, &MainWindow::onThemeChanged);
    connect(m_settingsWidget, &SettingsWidget::languageChanged, this, &MainWindow::onLanguageChanged);
    connect(m_settingsWidget, &SettingsWidget::devicesChanged, m_homeWidget, &HomeWidget::refreshDeviceStatus);
    connect(m_settingsWidget, &SettingsWidget::devicesChanged, m_homeWidget, &HomeWidget::refreshQuickControls);
    connect(m_settingsWidget, &SettingsWidget::devicesChanged, m_deviceControlWidget, &DeviceControlWidget::refreshDevices);
    connect(m_settingsWidget, &SettingsWidget::devicesChanged, m_historyWidget, &HistoryWidget::refreshData);
    connect(m_homeWidget, &HomeWidget::alarmTriggered, m_alarmWidget, &AlarmWidget::triggerAlarm);

    refreshNavIcons(false);
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

        // 强制重新抛光，避免主题来回切换后个别控件保留旧样式状态
        const QWidgetList widgets = qApp->allWidgets();
        for (QWidget *widget : widgets)
        {
            if (!widget)
            {
                continue;
            }
            widget->style()->unpolish(widget);
            widget->style()->polish(widget);
            widget->update();
        }

        refreshNavIcons(themeName == QStringLiteral("dark"));

        if (m_homeWidget)
        {
            m_homeWidget->refreshQuickControls();
        }
        if (m_deviceControlWidget)
        {
            m_deviceControlWidget->refreshDevices();
        }
    }
}

void MainWindow::refreshNavIcons(bool darkTheme)
{
    if (!ui || !ui->navBar)
    {
        return;
    }

    const QColor iconColor = darkTheme ? QColor("#D8E7FF") : QColor("#30485F");
    for (int i = 0; i < ui->navBar->count(); ++i)
    {
        QListWidgetItem *item = ui->navBar->item(i);
        if (!item)
        {
            continue;
        }

        const QString iconPath = item->data(Qt::UserRole).toString();
        if (!iconPath.trimmed().isEmpty())
        {
            item->setIcon(tintedIcon(iconPath, iconColor));
        }
    }
}

void MainWindow::applyLanguage(const QString &languageKey)
{
    m_languageKey = languageKey;
    qApp->removeTranslator(&m_translator);

    if (languageKey == QStringLiteral("en_US"))
    {
        if (m_translator.load(QStringLiteral(":/i18n/SmartHome_en_US")))
        {
            qApp->installTranslator(&m_translator);
        }
    }

    const bool isEnglish = (languageKey == QStringLiteral("en_US"));
    setWindowTitle(isEnglish ? QStringLiteral("Smart Home Platform") : QStringLiteral("智能家居监控平台"));
    const QStringList navTexts = isEnglish
                                     ? QStringList{QStringLiteral("Home"),
                                                   QStringLiteral("Devices"),
                                                   QStringLiteral("Scenes"),
                                                   QStringLiteral("History"),
                                                   QStringLiteral("Alarms"),
                                                   QStringLiteral("Settings")}
                                     : QStringList{QStringLiteral("首页"),
                                                   QStringLiteral("设备控制"),
                                                   QStringLiteral("场景管理"),
                                                   QStringLiteral("历史记录"),
                                                   QStringLiteral("报警设置"),
                                                   QStringLiteral("系统设置")};
    for (int i = 0; i < ui->navBar->count() && i < navTexts.size(); ++i)
    {
        ui->navBar->item(i)->setText(navTexts.at(i));
    }

    if (m_settingsWidget)
    {
        m_settingsWidget->applyLanguage(languageKey);
    }
    if (m_homeWidget)
    {
        m_homeWidget->applyLanguage(languageKey);
    }
    if (m_deviceControlWidget)
    {
        m_deviceControlWidget->applyLanguage(languageKey);
    }
    if (m_sceneWidget)
    {
        m_sceneWidget->applyLanguage(languageKey);
    }
    if (m_historyWidget)
    {
        m_historyWidget->applyLanguage(languageKey);
    }
    if (m_alarmWidget)
    {
        m_alarmWidget->applyLanguage(languageKey);
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
