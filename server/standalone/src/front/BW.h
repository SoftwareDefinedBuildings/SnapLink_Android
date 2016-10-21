#pragma once
#include "front/BWworker.h"
/*
#include "data/CameraModel.h"
#include "event/DetectionEvent.h"
#include "event/FailureEvent.h"
#include "event/QueryEvent.h"
#include "process/Identification.h"
#include "util/Time.h"
#include <QCoreApplication>
#include <cstdlib>
#include <string.h>
#include <strings.h>
#include "data/Session.h"
#include <QObject>
#include <QSemaphore>
#include <memory>
#include <mutex>
#include <opencv2/core/core.hpp>
#include <random>
#include <QCoreApplication>
#include </root/workspace/CellMate/server/third_party/qtlibbw/libbw.h>
#include <allocations.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <sstream>
#include <QThread>
#define MAX_CLIENTS 10
#define IMAGE_INIT_SIZE 100000
*/

/* BW_MSG have format:
    "Cellmate Image"
    str(identity)
    str(image contents)
    str(height)
    str(width)
    str(fx)
    str(fy)
    str(cx)
    str(cy)
*/

#define BW_MSG_LENGTH 9
#define DEFAULT_CHANNEL "scratch.ns/cellmate"


class CameraModel;
class FeatureStage;


typedef struct {
  double fx;
  double fy;
  double cx;
  double cy;
} CameraInfo;

typedef struct {
  QSemaphore detected;
  std::unique_ptr<std::vector<std::string>> names;
  std::unique_ptr<Session> session;
} ConnectionInfo;



class BWServer;
class BWWorker : public QObject
{ 
  Q_OBJECT
  public:
    BWWorker(){};
    virtual ~BWWorker(){};
public slots:
    void doWork(const PMessage &msg, BWServer *server);
signals:
    void doneWork(std::string result, std::string identity){};
  private:
    void workComplete(BWServer *server, ConnectionInfo *connInfo);
};



class BWServer : public QObject
{
  friend class BWWorker;
public:
  BWServer();
  virtual ~BWServer();

  void startRun();
  int getMaxClients() const;
  int getNumClients() const;
  void setNumClients(int numClients);
  void setFeatureStage(FeatureStage *featureStage);
public slots:
  void publishResult(std::string result, std::string identity);
signals:
  void askWorkerDoWork(const PMessage &msg, BWServer *server){};
protected:
  virtual bool event(QEvent *event);
  void agentChanged();
  QByteArray mustGetEntity();
  void parseMessage(PMessage msg);
private:
  static void createData(const std::vector<char> &data, double fx, double fy,
                         double cx, double cy, cv::Mat &image,
                         CameraModel &camera);
  BW *_bw;
  QByteArray _entity;
  unsigned int _maxClients;
  int _numClients;
  std::mutex _mutex;
  std::mt19937 _gen;
  std::uniform_int_distribution<unsigned long long> _dis;
  std::map<long, ConnectionInfo *> _connInfoMap;
  FeatureStage *_featureStage;
};
