#pragma once

#include "services/servicemodels.h"

#include <QString>

class EnvironmentService
{
public:
    EnvironmentSnapshot generateInitialSnapshot() const;
    EnvironmentSnapshot generateNextSnapshot() const;
    QString temperatureColor(double temperature) const;
};
