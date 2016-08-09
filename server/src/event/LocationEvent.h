#pragma once

#include <QEvent>
#include <memory>
#include <rtabmap/core/SensorData.h>
#include "data/Transform.h"
#include "data/PerfData.h"

class LocationEvent :
    public QEvent
{
public:
    // ownership transfer
    LocationEvent(int dbId, std::unique_ptr<rtabmap::SensorData> &&sensorData, std::unique_ptr<Transform> &&pose, std::unique_ptr<PerfData> &&perfData, const void *session);

    int dbId() const;
    std::unique_ptr<rtabmap::SensorData> takeSensorData();
    std::unique_ptr<Transform> takePose();
    std::unique_ptr<PerfData> takePerfData();
    const void *getSession();

    static QEvent::Type type();

private:
    static const QEvent::Type _type;
    const void *_session;
    int _dbId;
    std::unique_ptr<rtabmap::SensorData> _sensorData;
    std::unique_ptr<Transform> _pose;
    std::unique_ptr<PerfData> _perfData;
};
