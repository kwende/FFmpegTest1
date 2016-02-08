#pragma once
#include <queue>
#include "x264Encoder.h"
#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include "FramedSource.hh"
#include "GroupsockHelper.hh"
#include <Windows.h>
#include "KinectHelper.h"

class SimpleFramedSource : public FramedSource
{
public:
    static SimpleFramedSource* createNew(UsageEnvironment& env);
    // appears to be part of some eventing framework from live
    static EventTriggerId serverDataReadyEventId;
    static timeval GetLatestTimeVal(); 
    ~SimpleFramedSource();
    virtual void doGetNextFrame();
    virtual bool isCurrentlyAwaitingData(); 
    void pumpFrames(); 
    bool Running(); 
protected:
    SimpleFramedSource(UsageEnvironment& env);
    virtual void doStopGettingFrames();
private:
    x264Encoder *_encoder;
    std::queue<x264_nal_t> _nalQueue;
    static void onEventTriggered(void* clientData);
    void DeliverNALUnitsToLive555FromQueue(bool newData);
    void GetFrameAndEncodeToNALUnitsAndEnqueue(); 
    void EncodeAndDeliverFrameData(); 
    static timeval _time; 
    cv::Mat frame; 
    int m_fps;
    HANDLE _thread, _serverNeedDataEvent, _serverShutDownEvent;
    USHORT* _latestFrameData; 
    CRITICAL_SECTION _section; 
    bool _doThread; 
    bool _isUnprocessedFrameAvailable; 
};

