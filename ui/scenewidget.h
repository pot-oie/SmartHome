#pragma once
#include <QWidget>
#include <QJsonObject>

#include "services/sceneservice.h"

namespace Ui
{
    class SceneWidget;
}

class SceneWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SceneWidget(QWidget *parent = nullptr);
    ~SceneWidget();

signals:
    // 【向外发送】用户点击“激活该场景”时，发射打包好的批量指令，交给 TCP 发送
    void requestTriggerScene(const QJsonObject &sceneCmd);

public slots:
    // 【接收后端】接收场景执行的部分成功或失败的结果
    void updateSceneExecutionResult(const QJsonObject &resultData);

private slots:
    // 【UI 交互】增删改查场景按钮
    void on_btnAddScene_clicked();
    void on_btnDeleteScene_clicked();
    void on_btnActivateScene_clicked();
    // 【UI 交互】在当前场景下添加要联动的设备
    void on_btnAddDeviceToScene_clicked();
    // 【UI 交互】在当前场景下移除联动设备
    void on_btnRemoveDevice_clicked();
    // 【UI 交互】场景选择变化
    void updateSceneDetails(int row);

private:
    Ui::SceneWidget *ui;
    SceneService m_sceneService;
    SceneList m_scenes;

    void loadScenesFromDatabase(); // 从 SQLite 加载已存的场景配置
    void renderSceneList();
    void renderSceneDetails(const SceneDefinition &scene);
};
