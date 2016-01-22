// FFmpegTest1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "LibAVEncoder.h"

#include "opencv2/opencv.hpp"
#include <opencv2/core/core.hpp>

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"
#include "H264LiveServerMediaSession.h"

void Display(cv::Mat& mat)
{
    cv::imshow("MyVideo", mat);
    cv::waitKey(1);

    std::cout << "Poop" << std::endl;
}

int TestPlay()
{
    cv::VideoCapture cap("c:/users/brush/desktop/feynman.mp4");
    if (!cap.isOpened()) {
        std::cout << "Cannot open the video file" << std::endl;
        return -1;
    }

    //double count = cap.get(CV_CAP_PROP_FRAME_COUNT); //get the frame count
    //cap.set(CV_CAP_PROP_POS_FRAMES, count - 1); //Set index to last frame
    cv::namedWindow("MyVideo", CV_WINDOW_AUTOSIZE);

    while (1)
    {
        cv::Mat frame;
        bool success = cap.read(frame);
        if (!success) {
            std::cout << "Cannot read  frame " << std::endl;
            break;
        }
        Display(frame); 
        //if (cv::waitKey(0) == 27) break;
    }
}

int main()
{
    //TestPlay(); 

    TaskScheduler* taskSchedular = BasicTaskScheduler::createNew();
    BasicUsageEnvironment* usageEnvironment = BasicUsageEnvironment::createNew(*taskSchedular);
    RTSPServer* rtspServer = RTSPServer::createNew(*usageEnvironment, 8554, NULL);
    if (rtspServer == NULL)
    {
        *usageEnvironment << "Failed to create rtsp server ::" << usageEnvironment->getResultMsg() << "\n";
        exit(1);
    }

    std::string streamName = "usb1";
    ServerMediaSession* sms = ServerMediaSession::createNew(*usageEnvironment, streamName.c_str(), streamName.c_str(), "Live H264 Stream");
    H264LiveServerMediaSession *liveSubSession = H264LiveServerMediaSession::createNew(*usageEnvironment, true);
    sms->addSubsession(liveSubSession);
    rtspServer->addServerMediaSession(sms);
    char* url = rtspServer->rtspURL(sms);
    *usageEnvironment << "Play the stream using url " << url << "\n";
    delete[] url;
    taskSchedular->doEventLoop();

    return 0;
}

