#pragma once
#include <QWidget>
#include <QJsonObject>

namespace Ui
{
    class HomeWidget;
}

class HomeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWidget(QWidget *parent = nullptr);
    ~HomeWidget();

signals:
    // 【向外发送的信号】：当用户在界面上操作时触发，通知后端去干活
    void requestQuickControl(QString deviceId, QString cmd);

public slots:
    // 【接收后端的槽函数】：暴露给外部，当后端有数据时调用它来更新界面
    void updateEnvironmentData(double temp, double hum);
    void updateEnvironmentUI(const QJsonObject &data);

private slots:
    // 【UI 内部交互槽函数】：处理界面上按钮的点击等
    void on_btnGoHome_clicked();
    void onQuickControlClicked();

private:
    Ui::HomeWidget *ui;
    void initConnections();
};
