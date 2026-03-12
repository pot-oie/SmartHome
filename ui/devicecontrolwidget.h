#pragma once

#include <QJsonObject>
#include <QWidget>

#include "services/deviceservice.h"

namespace Ui
{
    class DeviceControlWidget;
}

class DeviceControlWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DeviceControlWidget(QWidget *parent = nullptr);
    ~DeviceControlWidget();
    void applyLanguage(const QString &languageKey);

signals:
    void requestControlDevice(const QJsonObject &controlCmd);

public slots:
    void updateDeviceStatus(const QJsonObject &statusData);
    void refreshDevices();

protected:
    void changeEvent(QEvent *event) override;

private slots:
    void on_listCategory_currentRowChanged(int currentRow);
    void onDeviceSwitchToggled(bool checked);
    void onDeviceSliderValueChanged(int value);

private:
    void initDeviceList();
    void reloadDevices(bool reloadCategories = false);
    void updateDeviceListUI(int category);
    void scheduleThemeRefresh();

private:
    Ui::DeviceControlWidget *ui;
    DeviceService m_deviceService;
    DeviceList m_allDevices;
    QStringList m_categories;
    QString m_languageKey = QStringLiteral("zh_CN");
    bool m_themeRefreshScheduled = false;
};
