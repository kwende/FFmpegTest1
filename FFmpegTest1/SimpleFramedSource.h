#pragma once
#include <queue>
#include "x264Encoder.h"
#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>
#include "FramedSource.hh"
#include "GroupsockHelper.hh"
#include <Windows.h>
#include <Kinect.h>

// InfraredSourceValueMaximum is the highest value that can be returned in the InfraredFrame.
// It is cast to a float for readability in the visualization code.
#define InfraredSourceValueMaximum static_cast<float>(USHRT_MAX)

// The InfraredOutputValueMinimum value is used to set the lower limit, post processing, of the
// infrared data that we will render.
// Increasing or decreasing this value sets a brightness "wall" either closer or further away.
#define InfraredOutputValueMinimum 0.01f 

// The InfraredOutputValueMaximum value is the upper limit, post processing, of the
// infrared data that we will render.
#define InfraredOutputValueMaximum 1.0f

// The InfraredSceneValueAverage value specifies the average infrared value of the scene.
// This value was selected by analyzing the average pixel intensity for a given scene.
// Depending on the visualization requirements for a given application, this value can be
// hard coded, as was done here, or calculated by averaging the intensity for each pixel prior
// to rendering.
#define InfraredSceneValueAverage 0.08f

/// The InfraredSceneStandardDeviations value specifies the number of standard deviations
/// to apply to InfraredSceneValueAverage. This value was selected by analyzing data
/// from a given scene.
/// Depending on the visualization requirements for a given application, this value can be
/// hard coded, as was done here, or calculated at runtime.
#define InfraredSceneStandardDeviations 3.0f

class SimpleFramedSource : public FramedSource
{
public:
    static SimpleFramedSource* createNew(UsageEnvironment& env);
    // appears to be part of some eventing framework from live
    static EventTriggerId eventTriggerId;

    ~SimpleFramedSource();
    virtual void doGetNextFrame();
    virtual bool isCurrentlyAwaitingData(); 
protected:
    SimpleFramedSource(UsageEnvironment& env);
private:
    USHORT* GetDepthBuffer(); 
    USHORT* GetIRBuffer();
    x264Encoder *_encoder;
    std::queue<x264_nal_t> _nalQueue;
    static void onEventTriggered(void* clientData);
    void DeliverNALUnitsToLive555FromQueue(bool newData);
    void GetFrameAndEncodeToNALUnitsAndEnqueue(); 
    int _currentFrameCount; 
    timeval _time; 
    cv::Mat frame;
    long _lastTickCount; 
    // Current Kinect
    IKinectSensor*          m_pKinectSensor;

    // Depth reader
    IDepthFrameReader*      m_pDepthFrameReader;
    IInfraredFrameReader*   m_pInfraredFrameReader;

    UINT16 *m_pCachedBuffer; 
    int m_fps;
};

