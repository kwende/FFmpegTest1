#include "stdafx.h"
#include "x264Encoder.h"
#include <Windows.h>

x264Encoder::x264Encoder()
{
    _fps = 30; 
    _waitPeriod = 1000 / _fps; 
    _lastEncodeTime = -1; 
}


x264Encoder::~x264Encoder()
{
}

void x264Encoder::Initilize()
{
    x264_param_default_preset(&parameters, "veryfast", "zerolatency");
    parameters.i_log_level = X264_LOG_INFO;
    parameters.i_threads = 1;
    parameters.i_width = 512 / 2;
    parameters.i_height = 424 / 2;
    parameters.i_fps_num = _fps;
    parameters.i_fps_den = 1;
    parameters.i_keyint_max = 15;
    parameters.b_intra_refresh = 1;
    parameters.rc.i_rc_method = X264_RC_CRF;
    parameters.rc.i_vbv_buffer_size = 1000000;
    parameters.rc.i_vbv_max_bitrate = 90000;
    parameters.rc.f_rf_constant = 25;
    parameters.rc.f_rf_constant_max = 35;
    parameters.i_sps_id = 7;
    // the following two value you should keep 1
    parameters.b_repeat_headers = 1;    // to get header before every I-Frame
    parameters.b_annexb = 1; // put start code in front of nal. we will remove start code later
    x264_param_apply_profile(&parameters, "baseline");

    encoder = x264_encoder_open(&parameters);

    x264_picture_alloc(&picture_in, X264_CSP_I420, 512 / 2, 424 / 2);
    picture_in.i_type = X264_TYPE_AUTO;
    picture_in.img.i_csp = X264_CSP_I420;

    x264_picture_alloc(&picture_out, X264_CSP_I420, parameters.i_width, parameters.i_height);
    picture_out.i_type = X264_TYPE_AUTO;
    picture_out.img.i_csp = X264_CSP_I420;

    // i have initilized my color space converter for BGR24 to YUV420 because my opencv 
    // video capture gives BGR24 image. You can initilize according to your input pixelFormat
    convertContext = sws_getContext(
        512 / 2,
        424 / 2,
        PIX_FMT_BGR24, 
        parameters.i_width, 
        parameters.i_height, 
        PIX_FMT_YUV420P, 
        SWS_FAST_BILINEAR, 
        NULL, NULL, NULL);
}

void x264Encoder::UnInitilize()
{
    x264_encoder_close(encoder);
    sws_freeContext(convertContext);
}

//void x264Encoder::EncodeFrame(const cv::Mat& image)
void x264Encoder::EncodeFrame(cv::Mat& image)
{
    int srcStride = 512 / 2 * 3;

    sws_scale(convertContext, &(image.data), &srcStride, 0, 424 / 2, picture_out.img.plane, picture_out.img.i_stride);
    x264_nal_t* nals;
    int i_nals = 0;
    int frameSize = -1;

    frameSize = x264_encoder_encode(encoder, &nals, &i_nals, &picture_out, &picture_out2);
    if (frameSize > 0)
    {
        for (int i = 0; i< i_nals; i++)
        {
            outputQueue.push(nals[i]);
        }
    }
}

bool x264Encoder::IsNalsAvailableInOutputQueue()
{
    if (outputQueue.empty() == true)
    {
        return false;
    }
    else
    {
        return true;
    }
}

x264_nal_t x264Encoder::getNalUnit()
{
    x264_nal_t nal;
    nal = outputQueue.front();
    outputQueue.pop();
    return nal;
}
