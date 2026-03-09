#include "scenewidget.h"
#include "ui_scenewidget.h"
#include <QDebug>

SceneWidget::SceneWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::SceneWidget)
{
    ui->setupUi(this);
    loadScenesFromDatabase();
}

SceneWidget::~SceneWidget()
{
    delete ui;
}

void SceneWidget::loadScenesFromDatabase() {
    // 从 SQLite 加载场景
}

void SceneWidget::updateSceneExecutionResult(const QJsonObject& resultData) {
    // 处理执行结果
}

void SceneWidget::on_btnAddScene_clicked() {
    qDebug() << "添加场景";
}

void SceneWidget::on_btnDeleteScene_clicked() {
    qDebug() << "删除场景";
}

void SceneWidget::on_btnActivateScene_clicked() {
    qDebug() << "激活场景";
}

void SceneWidget::on_btnAddDeviceToScene_clicked() {
    qDebug() << "向场景添加设备";
}
