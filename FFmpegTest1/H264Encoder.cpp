#include "stdafx.h"
#include "H264Encoder.h"


H264Encoder::H264Encoder()
{
    c = NULL;
    frame = NULL;
}


H264Encoder::~H264Encoder()
{
    Shutdown(); 
}

void H264Encoder::Shutdown()
{
    if (c != NULL && frame != NULL)
    {
        avcodec_close(c);
        av_free(c);

        av_freep(&frame->data[0]);
        av_frame_free(&frame);

        c = NULL;
        frame = NULL;
    }
}

void H264Encoder::EncodeFrame(cv::Mat& image)
{

}

bool H264Encoder::IsNalsAvailableInOutputQueue()
{
    return false; 
}

x264_nal_t H264Encoder::getNalUnit()
{
    x264_nal_t ret;
    ret.b_long_startcode = 0; 

    return ret; 
}

void H264Encoder::DoLoop()
{
    int ret = 0, x = 0, y = 0, got_output = 0, i = 0;

    /* encode 1 second of video */
    for (i = 0; i < 25; i++) {
        av_init_packet(&pkt);
        pkt.data = NULL;    // packet data will be allocated by the encoder
        pkt.size = 0;
        fflush(stdout);
        /* prepare a dummy image */
        /* Y */
        for (y = 0; y < c->height; y++) {
            for (x = 0; x < c->width; x++) {
                frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
            }
        }
        /* Cb and Cr */
        for (y = 0; y < c->height / 2; y++) {
            for (x = 0; x < c->width / 2; x++) {
                frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
            }
        }
        frame->pts = i;
        /* encode the image */
        ret = avcodec_encode_video2(c, &pkt, frame, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            //fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }
    /* get the delayed frames */
    for (got_output = 1; got_output; i++) {
        fflush(stdout);
        ret = avcodec_encode_video2(c, &pkt, NULL, &got_output);
        if (ret < 0) {
            fprintf(stderr, "Error encoding frame\n");
            exit(1);
        }
        if (got_output) {
            printf("Write frame %3d (size=%5d)\n", i, pkt.size);
            //fwrite(pkt.data, 1, pkt.size, f);
            av_free_packet(&pkt);
        }
    }
}

void H264Encoder::Initialize()
{
    avcodec_register_all();

    char* filename = "c:/users/brush/desktop/test.h264";
    AVCodecID codec_id = AVCodecID::AV_CODEC_ID_H264;

    AVCodec *codec;
    
    int ret = 0;
    //FILE *f;

    uint8_t endcode[] = { 0, 0, 1, 0xb7 };
    printf("Encode video file %s\n", filename);
    /* find the mpeg1 video encoder */
    codec = avcodec_find_encoder(codec_id);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        exit(1);
    }
    c = avcodec_alloc_context3(codec);
    if (!c) {
        fprintf(stderr, "Could not allocate video codec context\n");
        exit(1);
    }
    /* put sample parameters */
    c->bit_rate = 400000;
    /* resolution must be a multiple of two */
    c->width = 352;
    c->height = 288;
    /* frames per second */
    AVRational rational;
    rational.num = 1;
    rational.den = 25;
    c->time_base = rational;
    /* c->time_base = (AVRational) { 1, 25 };*/
    c->gop_size = 10; /* emit one intra frame every ten frames */
    c->max_b_frames = 1;
    c->pix_fmt = AV_PIX_FMT_YUV420P;
    if (codec_id == AV_CODEC_ID_H264)
        av_opt_set(c->priv_data, "preset", "slow", 0);
    /* open it */
    if (avcodec_open2(c, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        exit(1);
    }
    //f = fopen(filename, "wb");
    //if (!f) {
    //    fprintf(stderr, "Could not open %s\n", filename);
    //    exit(1);
    //}
    frame = av_frame_alloc();
    if (!frame) {
        fprintf(stderr, "Could not allocate video frame\n");
        exit(1);
    }
    frame->format = c->pix_fmt;
    frame->width = c->width;
    frame->height = c->height;
    /* the image can be allocated by any means and av_image_alloc() is
    * just the most convenient way if av_malloc() is to be used */
    ret = av_image_alloc(frame->data, frame->linesize, c->width, c->height,
        c->pix_fmt, 32);
    if (ret < 0) {
        fprintf(stderr, "Could not allocate raw picture buffer\n");
        exit(1);
    }


}
