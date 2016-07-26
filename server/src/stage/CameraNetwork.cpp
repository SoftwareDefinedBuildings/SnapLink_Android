#include <rtabmap/utilite/UTimer.h>
#include <rtabmap/utilite/ULogger.h>
#include <cstdio>
#include <opencv2/imgproc/imgproc.hpp>
#include <QCoreApplication>

#include "stage/CameraNetwork.h"
#include "event/NetworkEvent.h"
#include "event/ImageEvent.h"
#include "event/FailureEvent.h"

CameraNetwork::CameraNetwork() :
    _feature(nullptr),
    _httpServer(nullptr)
{
}

CameraNetwork::~CameraNetwork()
{
    _feature = nullptr;
    _httpServer = nullptr;
}

void CameraNetwork::setFeatureExtraction(FeatureExtraction *feature)
{
    _feature = feature;
}

void CameraNetwork::setHTTPServer(HTTPServer *httpServer)
{
    _httpServer = httpServer;
}

bool CameraNetwork::event(QEvent *event)
{
    if (event->type() == NetworkEvent::type())
    {
        NetworkEvent *networkEvent = static_cast<NetworkEvent *>(event);

        const std::vector<char> &data = networkEvent->sessionInfo()->data;
        double fx = networkEvent->sessionInfo()->cameraInfo.fx;
        double fy = networkEvent->sessionInfo()->cameraInfo.fy;
        double cx = networkEvent->sessionInfo()->cameraInfo.cx;
        double cy = networkEvent->sessionInfo()->cameraInfo.cy;
        std::unique_ptr<SessionInfo> sessionInfo(networkEvent->sessionInfo());

        std::unique_ptr<rtabmap::SensorData> sensorData = createSensorData(data, fx, fy, cx, cy);
        if (sensorData != nullptr)
        {
            QCoreApplication::postEvent(_feature, new ImageEvent(std::move(sensorData), networkEvent->sessionInfo()));
        }
        else
        {
            QCoreApplication::postEvent(_httpServer, new FailureEvent(std::move(sessionInfo)));
        }
        return true;
    }
    return QObject::event(event);
}

std::unique_ptr<rtabmap::SensorData> CameraNetwork::createSensorData(const std::vector<char> &data, double fx, double fy, double cx, double cy) const
{
    UDEBUG("");

    // there is no data copy here, the cv::Mat has a pointer to the data
    const bool copyData = false;
    cv::Mat img = imdecode(cv::Mat(data, copyData), cv::IMREAD_GRAYSCALE);

    if (img.empty())
    {
        return nullptr;
    }

    //imwrite("image.jpg", img);

    if (!img.empty())
    {
        rtabmap::Transform localTransform(0, 0, 1, 0, -1, 0, 0, 0, 0, -1, 0, 0);
        rtabmap::CameraModel model(fx, fy, cx, cy, localTransform);
        std::unique_ptr<rtabmap::SensorData> sensorData(new rtabmap::SensorData(img, model));
        return sensorData;
    }

    return nullptr;
}
