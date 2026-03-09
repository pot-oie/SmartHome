#pragma once

#include "services/servicemodels.h"

#include <QJsonObject>

class SceneService
{
public:
    SceneList loadDefaultScenes() const;
    SceneDefinition createCustomScene(const QString &sceneName) const;
    QJsonObject buildTriggerSceneCommand(const SceneDefinition &scene) const;
};
