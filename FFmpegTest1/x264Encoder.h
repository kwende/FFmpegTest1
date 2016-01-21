#pragma once
#ifdef __cplusplus
extern "C" {
#endif

#include "libavutil/opt.h"
#include "libavcodec/avcodec.h"
#include "libavutil/channel_layout.h"
#include "libavutil/common.h"
#include "libavutil/imgutils.h"
#include "libavutil/mathematics.h"
#include "libavutil/samplefmt.h"
#include "libswscale\swscale.h"
//
#include "x264.h"

#ifdef __cplusplus
}
#endif

#include <iostream>
#include <concurrent_queue.h>
#include <queue>

#include <opencv2\opencv.hpp>


class x264Encoder
{
public:
    x264Encoder();
    ~x264Encoder();

    void Initilize();
    void UnInitilize();
    void EncodeFrame(cv::Mat& image);
    bool IsNalsAvailableInOutputQueue();
    x264_nal_t getNalUnit();
private:
    SwsContext* convertContext;
    std::queue<x264_nal_t> outputQueue;
    x264_param_t parameters;
    x264_picture_t picture_in, picture_out;
    x264_t* encoder;
};

