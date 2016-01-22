#pragma once
#include <queue>
#include "x264Encoder.h"
#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include "FramedSource.hh"
#include "GroupsockHelper.hh"
#include <Windows.h>
#include <Kinect.h>

class SimpleFramedSource : public FramedSource
{
public:
    static SimpleFramedSource* createNew(UsageEnvironment& env);
    // appears to be part of some eventing framework from live
    static EventTriggerId eventTriggerId;

    ~SimpleFramedSource();
    virtual void doGetNextFrame();
protected:
    SimpleFramedSource(UsageEnvironment& env);
private:
    x264Encoder *_encoder;
    std::queue<x264_nal_t> _nalQueue;
    static void onEventTriggered(void* clientData);
    void DeliverNALUnitsToLive555FromQueue();
    void GetFrameAndEncodeToNALUnitsAndEnqueue(); 
    int _currentFrameCount; 
    cv::VideoCapture *_videoCap; 
    timeval _time; 
    cv::Mat frame;
    long _lastTickCount; 
    // Current Kinect
    IKinectSensor*          m_pKinectSensor;

    // Depth reader
    IDepthFrameReader*      m_pDepthFrameReader;
};

