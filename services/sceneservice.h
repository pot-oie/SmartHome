#pragma once

#include "services/servicemodels.h"

#include <QJsonObject>

class SceneService
{
public:
    SceneList loadDefaultScenes() const;
    SceneDefinition createCustomScene(const QString &sceneName) const;
    SceneDefinition createScene(const QString &sceneName, const QString &sceneDescription, const QString &iconPath = ":/icons/scene.svg") const;
    bool addDeviceAction(const SceneDefinition &scene, const SceneDeviceAction &action) const;
    bool removeDeviceAction(const SceneDefinition &scene, const SceneDeviceAction &action) const;
    bool deleteScene(const SceneDefinition &scene) const;
    QJsonObject buildTriggerSceneCommand(const SceneDefinition &scene) const;
};
