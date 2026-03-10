#pragma once

#include "../../services/servicemodels.h"

#include <QString>

class SceneDao
{
public:
    SceneList listScenesWithActions();
    bool insertScene(SceneDefinition &scene);
    bool updateScene(const SceneDefinition &scene);
    bool insertSceneAction(const QString &sceneCode, const SceneDeviceAction &action);
    bool updateSceneAction(const QString &sceneCode, const SceneDeviceAction &oldAction, const SceneDeviceAction &newAction);
    bool deleteSceneAction(const QString &sceneCode, const SceneDeviceAction &action);
    bool deleteSceneByCode(const QString &sceneCode);
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
