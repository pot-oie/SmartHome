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
#include <QStyleOptionViewItem>
#include <QStyle>
#include <QTimer>
#include <QGuiApplication>
#include <QScreen>

namespace
{
    qreal iconDevicePixelRatio()
    {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen)
        {
            return 1.0;
        }
        return qMax<qreal>(1.0, screen->devicePixelRatio());
    }

    QIcon tintedIcon(const QString &path, const QColor &color)
    {
        const QIcon baseIcon(path);
        const QSize canvasSize(24, 24);
        const int drawSize = 20;
        const qreal dpr = iconDevicePixelRatio();
        const QSize srcPixelSize(qMax(1, qRound(drawSize * dpr)),
                                 qMax(1, qRound(drawSize * dpr)));
        const QPixmap src = baseIcon.pixmap(srcPixelSize);

        if (src.isNull())
        {
            return baseIcon;
        }

        const QSize canvasPixelSize(qMax(1, qRound(canvasSize.width() * dpr)),
                                    qMax(1, qRound(canvasSize.height() * dpr)));
        QPixmap tinted(canvasPixelSize);
        tinted.setDevicePixelRatio(dpr);
        tinted.fill(Qt::transparent);
        QPainter painter(&tinted);
        painter.setRenderHint(QPainter::Antialiasing); // 开启抗锯齿让缩小后的 SVG 更平滑
        painter.setRenderHint(QPainter::SmoothPixmapTransform, true);

        const qreal x = (canvasSize.width() - drawSize) / 2.0;

        int yOffset = 0.8;
        const qreal y = (canvasSize.height() - drawSize) / 2.0 + yOffset;

        // 带着偏移量绘制原始图标
        painter.drawPixmap(QRectF(x, y, drawSize, drawSize), src, QRectF(0, 0, src.width(), src.height()));
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRectF(QPointF(0, 0), QSizeF(canvasSize)), color);
        painter.end();
        return QIcon(tinted);
    }
}

NavBarItemDelegate::NavBarItemDelegate(QObject *parent)
    : QStyledItemDelegate(parent)
{
}

void NavBarItemDelegate::paint(QPainter *painter,
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const
{
    QStyleOptionViewItem styleOption(option);
    initStyleOption(&styleOption, index);

    QStyle *style = styleOption.widget ? styleOption.widget->style() : QApplication::style();

    QStyleOptionViewItem panelOption(styleOption);
    panelOption.text.clear();
    panelOption.icon = QIcon();
    style->drawPrimitive(QStyle::PE_PanelItemViewItem, &panelOption, painter, panelOption.widget);

    const QRect itemRect = styleOption.rect;

    bool darkTheme = false;
    if (styleOption.widget && styleOption.widget->property("isDarkTheme").isValid())
    {
        darkTheme = styleOption.widget->property("isDarkTheme").toBool();
    }
    else
    {
        darkTheme = styleOption.palette.color(QPalette::Base).lightness() < 128;
    }

    const QColor textColor = darkTheme ? QColor("#D8E7FF") : QColor("#223B56");
    const QSize decorationSize = styleOption.decorationSize.isValid() ? styleOption.decorationSize : QSize(24, 24);
    QFont textFont = styleOption.font;
    textFont.setBold(styleOption.state.testFlag(QStyle::State_Selected));
    QFontMetrics fontMetrics(textFont);
    const int textLeft = itemRect.left() + 58;
    const int textWidth = qMax(0, itemRect.right() - textLeft - 20);
    const QString text = fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideRight, textWidth);
    const int contentCenterY = itemRect.center().y();
    const int baselineY = contentCenterY + (fontMetrics.ascent() - fontMetrics.descent()) / 2;
    const int iconSize = qMin(qMin(decorationSize.width(), decorationSize.height()), qMax(20, itemRect.height() - 28));
    const int iconLeft = itemRect.left() + 25;
    QRect iconRect(iconLeft, contentCenterY - iconSize / 2, iconSize, iconSize);

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, true);
    painter->setRenderHint(QPainter::SmoothPixmapTransform, true);

    const QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
    if (!icon.isNull())
    {
        icon.paint(painter, iconRect, Qt::AlignCenter, QIcon::Normal, QIcon::Off);
    }

    painter->setFont(textFont);
    painter->setPen(textColor);
    painter->drawText(QPoint(textLeft, baselineY), text);
    painter->restore();
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
    ui->navBar->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->navBar->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->navBar->setUniformItemSizes(false);
    ui->navBar->setSelectionRectVisible(false);
    ui->navBar->setItemDelegate(new NavBarItemDelegate(ui->navBar));

    auto addNavItem = [this](const QString &icon, const QString &text)
    {
        QListWidgetItem *item = new QListWidgetItem(QIcon(icon), text);
        item->setData(Qt::UserRole, icon);
        item->setSizeHint(QSize(200, 80));
        item->setTextAlignment(Qt::AlignVCenter | Qt::AlignLeft);
        QFont navFont = item->font();
        navFont.setPointSize(11);

        navFont.setWeight(QFont::DemiBold);

        item->setFont(navFont);
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
    updateNavBarLayout();
    QTimer::singleShot(0, this, [this]()
                       { updateNavBarLayout(); });
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_homeWidget, &HomeWidget::refreshQuickControls);
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_homeWidget, &HomeWidget::refreshDeviceStatus);
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_deviceControlWidget, &DeviceControlWidget::refreshDevices);
    connect(m_sceneWidget, &SceneWidget::sceneExecuted, m_historyWidget, &HistoryWidget::refreshData);
}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    updateNavBarLayout();
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

        bool isDark = (themeName == QStringLiteral("dark"));
        ui->navBar->setProperty("isDarkTheme", isDark);

        // 仅重抛光主窗口及其子树，避免全局遍历导致主题切换卡顿。
        QWidgetList widgets;
        widgets << this;
        widgets.append(this->findChildren<QWidget *>());
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
        updateNavBarLayout();

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

void MainWindow::updateNavBarLayout()
{
    if (!ui || !ui->navBar)
    {
        return;
    }

    const int itemCount = ui->navBar->count();
    if (itemCount <= 0)
    {
        return;
    }

    const int itemWidth = qMax(150, ui->navBar->viewport()->width() - 4);
    for (int i = 0; i < itemCount; ++i)
    {
        QListWidgetItem *item = ui->navBar->item(i);
        if (!item)
        {
            continue;
        }

        item->setSizeHint(QSize(itemWidth, 80));
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

    updateNavBarLayout();

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
