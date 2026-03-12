#pragma once
#include <QMainWindow>
#include <QResizeEvent>
#include <QStyledItemDelegate>
#include <QTranslator>

QT_BEGIN_NAMESPACE
namespace Ui
{
    class MainWindow;
}
QT_END_NAMESPACE

class HomeWidget;
class DeviceControlWidget;
class AlarmWidget;
class SettingsWidget;
class SceneWidget;
class HistoryWidget;

class NavBarItemDelegate : public QStyledItemDelegate
{
public:
    explicit NavBarItemDelegate(QObject *parent = nullptr);

    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void onNavBarItemClicked(int index);
    void onThemeChanged(const QString &themeName);
    void onLanguageChanged(const QString &languageKey);

private:
    void initUI();
    void applyTheme(const QString &themeName);
    void applyLanguage(const QString &languageKey);
    QString loadStyleSheet(const QString &resourcePath) const;
    void refreshNavIcons(bool darkTheme);
    void updateNavBarLayout();
    Ui::MainWindow *ui;
    HomeWidget *m_homeWidget = nullptr;
    DeviceControlWidget *m_deviceControlWidget = nullptr;
    SceneWidget *m_sceneWidget = nullptr;
    HistoryWidget *m_historyWidget = nullptr;
    AlarmWidget *m_alarmWidget = nullptr;
    SettingsWidget *m_settingsWidget = nullptr;
    QTranslator m_translator;
    QString m_languageKey = QStringLiteral("zh_CN");
};
