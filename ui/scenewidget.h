#pragma once

#include <QEvent>
#include <QResizeEvent>
#include <QWidget>

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

public slots:
    void refreshScenes();

signals:
    void sceneExecuted();

private slots:
    void on_btnAddScene_clicked();
    void on_btnDeleteScene_clicked();
    void on_btnActivateScene_clicked();
    void on_btnAddDeviceToScene_clicked();
    void on_btnEditDeviceInScene_clicked();
    void on_btnRemoveDevice_clicked();
    void updateSceneDetails(int row);
    void editSelectedScene();
    void editSelectedAction(int row, int column);

private:
    void loadScenesFromDatabase(const QString &sceneCodeToSelect = QString(), qint64 actionIdToSelect = 0);
    void renderSceneList();
    void renderSceneDetails(const SceneDefinition &scene);
    void updateActionButtonsLayout();
    bool openSceneDialog(SceneDefinition *scene, const QString &title);
    bool openActionDialog(SceneDeviceAction *action, const QString &title);
    int findSceneRowByCode(const QString &sceneCode) const;
    int findActionRowById(const SceneDefinition &scene, qint64 actionId) const;

private:
    Ui::SceneWidget *ui;
    SceneService m_sceneService;
    SceneList m_scenes;
    QString m_languageKey = QStringLiteral("zh_CN");

protected:
    void changeEvent(QEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
};
