#pragma once

#include "../../services/servicemodels.h"

#include <QString>

class SceneDao
{
public:
    SceneList listScenesWithActions();
    bool insertScene(SceneDefinition &scene);
    bool insertSceneAction(const QString &sceneCode, const SceneDeviceAction &action);
    bool deleteSceneAction(const QString &sceneCode, const SceneDeviceAction &action);
    bool deleteSceneByCode(const QString &sceneCode);
    QString lastErrorText() const;

private:
    void setLastError(const QString &errorText);
    void clearLastError();

private:
    QString m_lastErrorText;
};
