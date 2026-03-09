#pragma once
#include <QWidget>
#include <QJsonObject>

namespace Ui {
class AlarmWidget;
}

class AlarmWidget : public QWidget {
    Q_OBJECT

public:
    explicit AlarmWidget(QWidget *parent = nullptr);
    ~AlarmWidget();

public slots:
    // 【接收后端】底层判定数据超标或设备故障时，调用此槽，触发声光报警
    void triggerAlarm(const QJsonObject& alarmData);

private slots:
    // 【UI 交互】保存设定的温湿度报警阈值
    void on_btnSaveThresholds_clicked();
    // 【UI 交互】清除历史报警记录
    void on_btnClearLogs_clicked();

private:
    Ui::AlarmWidget *ui;
    void loadThresholds(); // 界面初始化时，从配置或数据库读取当前阈值
};
