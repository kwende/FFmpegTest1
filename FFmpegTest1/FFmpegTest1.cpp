// FFmpegTest1.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "H264Encoder.h"


#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "GroupsockHelper.hh"

int main()
{
    H264Encoder encoder; 

    encoder.Initialize(); 
    encoder.DoLoop(); 
    encoder.Shutdown(); 

    return 0;
}

