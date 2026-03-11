#pragma once
#include <QMainWindow>
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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void onNavBarItemClicked(int index);
    void onThemeChanged(const QString &themeName);
    void onLanguageChanged(const QString &languageKey);

private:
    void initUI();
    void applyTheme(const QString &themeName);
    void applyLanguage(const QString &languageKey);
    QString loadStyleSheet(const QString &resourcePath) const;
    Ui::MainWindow *ui;
    HomeWidget *m_homeWidget = nullptr;
    DeviceControlWidget *m_deviceControlWidget = nullptr;
    AlarmWidget *m_alarmWidget = nullptr;
    QTranslator m_translator;
};
