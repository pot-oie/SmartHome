#pragma once
#include <QWidget>
#include <QJsonObject> // 引入 JSON，用于和后端的 TCP 协议交互

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

signals:
    // 【向外发送】当用户操作了界面（如开灯、调节空调），发出此信号，由 NetworkManager 捕获并转为 TCP 发送
    void requestControlDevice(const QJsonObject &controlCmd);

public slots:
    // 【接收后端】当底层 TCP 收到设备状态变化（如设备离线、被其他终端打开）时，调用此槽更新 UI
    void updateDeviceStatus(const QJsonObject &statusData);

private slots:
    // 【UI 交互】左侧设备分类列表切换时触发
    void on_listCategory_currentRowChanged(int currentRow);
    // 【UI 交互】某个设备的开关按钮被点击时触发（复用槽函数）
    void onDeviceSwitchToggled(bool checked);
    // 【UI 交互】空调温度/灯光亮度滑块拖动时触发
    void onDeviceSliderValueChanged(int value);

private:
    Ui::DeviceControlWidget *ui;
    DeviceService m_deviceService;
    DeviceList m_allDevices;

    void initDeviceList();                 // 初始化界面的辅助函数
    void updateDeviceListUI(int category); // 更新设备列表显示
    QStringList m_categories;              // 设备分类列表
};
