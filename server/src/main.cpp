#include <rtabmap/utilite/UConversion.h>
#include <rtabmap/core/RtabmapThread.h>
#include <rtabmap/core/Odometry.h>
#include <rtabmap/core/Parameters.h>
#include <rtabmap/utilite/UEventsManager.h>
#include <cstdio>
#include <QCoreApplication>
#include <QThread>
#include "data/WordsKdTree.h"
#include "data/SignaturesSimple.h"
#include "data/LabelsSimple.h"
#include "stage/HTTPServer.h"
#include "stage/FeatureExtraction.h"
#include "stage/WordSearch.h"
#include "stage/SignatureSearch.h"
#include "stage/Perspective.h"
#include "stage/CameraNetwork.h"
#include "stage/Visibility.h"
#include "adapter/RTABMapDBAdapter.h"


void showUsage()
{
    printf("\nUsage:\n"
           "CellMate database_file1 [database_file2 ...]\n");
    exit(1);
}

int main(int argc, char *argv[])
{
    ULogger::setType(ULogger::kTypeConsole);
    //ULogger::setLevel(ULogger::kInfo);
    ULogger::setLevel(ULogger::kDebug);

    std::vector<std::string> dbfiles;
    for (int i = 1; i < argc; i++)
    {
        dbfiles.push_back(std::string(argv[i]));
    }

    QCoreApplication app(argc, argv);

    WordsKdTree words;
    SignaturesSimple signatures;
    LabelsSimple labels;
    HTTPServer httpServer;
    CameraNetwork camera;
    FeatureExtraction feature;
    WordSearch wordSearch;
    SignatureSearch signatureSearch;
    Perspective perspective;
    Visibility vis;

    QThread cameraThread;
    QThread featureThread;
    QThread wordSearchThread;
    QThread signatureSearchThread;
    QThread perspectiveThread;
    QThread visThread;

    rtabmap::ParametersMap params;
    params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpDetectorStrategy(), uNumber2Str(rtabmap::Feature2D::kFeatureSurf)));
    params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kVisMinInliers(), "3"));
    params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpIncrementalDictionary(), "false")); // do not create new word because we don't know whether extedning BOW dimension is good...
    params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpNewWordsComparedTogether(), "false")); // do not compare with last signature's words
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpNNStrategy(), uNumber2Str(rtabmap::VWDictionary::kNNBruteForce))); // bruteforce
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpNndrRatio(), "0.3"));
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpMaxFeatures(), "50000"));
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpBadSignRatio(), "0"));
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kKpRoiRatios(), "0.0 0.0 0.0 0.0"));
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kMemGenerateIds(), "true"));
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kVisIterations(), "2000"));
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kVisPnPReprojError(), "1.0"));
    // params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kVisPnPFlags(), "0")); // 0=Iterative, 1=EPNP, 2=P3P
    params.insert(rtabmap::ParametersPair(rtabmap::Parameters::kSURFGpuVersion(), "true"));

    UINFO("Reading data");
    if (!RTABMapDBAdapter::readData(dbfiles, words, signatures, labels))
    {
        UERROR("Reading data failed");
        return 1;
    }

    // Visibility
    UINFO("Initializing Visibility");
    vis.setLabels(&labels);
    vis.setHTTPServer(&httpServer);
    vis.moveToThread(&visThread);
    visThread.start();

    // Perspective
    UINFO("Initializing Perspective");
    perspective.setHTTPServer(&httpServer);
    perspective.setVisibility(&vis);
    if (!perspective.init(params))
    {
        UERROR("Initializing Perspective failed");
        return 1;
    }
    perspective.moveToThread(&perspectiveThread);
    perspectiveThread.start();

    // Signature Search
    UINFO("Initializing Signature Search");
    signatureSearch.setSignatures(&signatures);
    signatureSearch.setPerspective(&perspective);
    signatureSearch.moveToThread(&signatureSearchThread);
    signatureSearchThread.start();

    // Word Search
    UINFO("Initializing Word Search");
    wordSearch.setWords(&words);
    wordSearch.setSignatureSearch(&signatureSearch);
    wordSearch.moveToThread(&wordSearchThread);
    wordSearchThread.start();

    // FeatureExtraction
    UINFO("Initializing feature extraction");
    feature.init(params);
    feature.setWordSearch(&wordSearch);
    feature.moveToThread(&featureThread);
    featureThread.start();

    // CameraNetwork
    UINFO("Initializing camera");
    camera.setHTTPServer(&httpServer);
    camera.setFeatureExtraction(&feature);
    camera.moveToThread(&cameraThread);
    cameraThread.start();

    // HTTPServer
    UINFO("Initializing HTTP server");
    httpServer.setCamera(&camera);
    if (!httpServer.start())
    {
        UERROR("Starting HTTP Server failed");
        return 1;
    }

    return app.exec();
}