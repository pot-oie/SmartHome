#pragma once

#include <QHideEvent>
#include <QJsonObject>
#include <QShowEvent>
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
    void refreshDevices();

protected:
    void changeEvent(QEvent *event) override;
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

private slots:
    void on_listCategory_currentRowChanged(int currentRow);
    void onDeviceSwitchToggled(bool checked);
    void onDeviceSliderValueChanged(int value);
    void onDevicesRefreshed(DeviceList devices);

private:
    void initDeviceList();
    void updateDeviceListUI(int category);
    void scheduleThemeRefresh();

private:
    Ui::DeviceControlWidget *ui;
    DeviceService m_deviceService;
    DeviceList m_allDevices;
    QStringList m_categories;
    QString m_languageKey = QStringLiteral("zh_CN");
    bool m_themeRefreshScheduled = false;
    QString m_lastDeviceRenderSignature;
    int m_lastRenderedCategory = -1;
};
