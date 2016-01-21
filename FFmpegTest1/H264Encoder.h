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

#ifdef __cplusplus
}
#endif

#include <iostream>
#include <concurrent_queue.h>
#include <queue>

#include <opencv2\opencv.hpp>

#include "x264.h"

class H264Encoder
{
public:
    H264Encoder();
    ~H264Encoder();

    void Initialize(); 
    void Shutdown(); 
    void EncodeFrame(cv::Mat& image);
    bool IsNalsAvailableInOutputQueue();
    x264_nal_t getNalUnit(); 
    void DoLoop();

private:
    AVCodecContext *c;
    AVFrame *frame;
    AVPacket pkt;
};

