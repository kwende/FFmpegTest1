// FFmpegTest1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"

#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "H264LiveServerMediaSession.h"
#include "WindowsAudioMediaSession.h"

int main()
{

    TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
    BasicUsageEnvironment* usageEnvironment = BasicUsageEnvironment::createNew(*taskSchedular);
    RTSPServer* rtspServer = RTSPServer::createNew(*usageEnvironment, 8554, NULL);
    if (rtspServer == NULL)
    {
        *usageEnvironment << "Failed to create rtsp server ::" << usageEnvironment->getResultMsg() << "\n";
        exit(1);
    }

    std::string streamName = "feynman";
    ServerMediaSession* sms = ServerMediaSession::createNew(*usageEnvironment, streamName.c_str(), streamName.c_str(), "Live H264 Stream");

    H264LiveServerMediaSession *liveSubSession = H264LiveServerMediaSession::createNew(*usageEnvironment, false);
    //sms->addSubsession(liveSubSession);
    WindowsAudioMediaSession* audioSession = WindowsAudioMediaSession::createNew(*usageEnvironment, false);
    sms->addSubsession(audioSession);
    rtspServer->addServerMediaSession(sms);
    char* url = rtspServer->rtspURL(sms);
    *usageEnvironment << "Play the stream using url " << url << "\n";
    delete[] url;
    taskSchedular->doEventLoop();

    return 0;
}

