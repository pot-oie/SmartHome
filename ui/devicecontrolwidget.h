#pragma once

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

public slots:
    void refreshDevices();

private slots:
    void on_listCategory_currentRowChanged(int currentRow);
    void onDeviceSwitchToggled(bool checked);
    void onDeviceSliderValueChanged(int value);

private:
    void initDeviceList();
    void reloadDevices(bool reloadCategories = false);
    void updateDeviceListUI(int category);

private:
    Ui::DeviceControlWidget *ui;
    DeviceService m_deviceService;
    DeviceList m_allDevices;
    QStringList m_categories;
};
