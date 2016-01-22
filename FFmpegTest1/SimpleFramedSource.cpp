#include "stdafx.h"
#include "SimpleFramedSource.h"

EventTriggerId SimpleFramedSource::eventTriggerId = 0;

SimpleFramedSource* SimpleFramedSource::createNew(UsageEnvironment& env)
{
    return new SimpleFramedSource(env); 
}

SimpleFramedSource::SimpleFramedSource(UsageEnvironment& env)
    :FramedSource(env)
{
    this->_videoCap = new cv::VideoCapture("c:/users/brush/desktop/feynman.mp4"); 
    this->_currentFrameCount = 0; 
    // start up the x264 encoder. 
    this->_encoder = new x264Encoder();
    this->_encoder->Initilize(); 
    // I'm stil not sure I completely understand what this trigger is for; I'm sure
    // I'll understand more once I see it in action. 
    if (eventTriggerId == 0)
    {
        eventTriggerId = envir().taskScheduler().createEventTrigger(onEventTriggered);
    }
}

// This is an important integration into the live 555 pipeline. This function appears to be the 
// dude that's called from the above classes to encode frame data and prepare it for pumping into
// the live 555 network shtick. 
void SimpleFramedSource::doGetNextFrame()
{
    if (this->_nalQueue.empty())
    {
        // get a frame of data, encode, and enqueue it. 
        this->GetFrameAndEncodeToNALUnitsAndEnqueue(); 
        // get time of day for the broadcaster
        gettimeofday(&_time, NULL); 
        // take the nal units and push them to live 555. 
        this->DeliverNALUnitsToLive555FromQueue(); 
    }
    else
    {
        // there's already stuff to deliver, so just deliver it. 
        this->DeliverNALUnitsToLive555FromQueue(); 
    }
}

// this is the guy which will take data, encode it, and push it to the NAL queue. 
void SimpleFramedSource::GetFrameAndEncodeToNALUnitsAndEnqueue()
{
    this->_currentFrameCount++; 

    // if we have encountered the last frame, start over. 
    if (this->_currentFrameCount >= this->_videoCap->get(CV_CAP_PROP_FRAME_COUNT))
    {
        this->_videoCap->set(CV_CAP_PROP_POS_FRAMES, 0); 
    }

    if (this->_videoCap->read(frame))
    {
        // encode to NALs
        this->_encoder->EncodeFrame(frame); 
        // now we dequeue the NAL units and put them onto our own queue. 
        while (this->_encoder->IsNalsAvailableInOutputQueue())
        {
            x264_nal_t nal = this->_encoder->getNalUnit();
            this->_nalQueue.push(nal);
        }
    }
}

// Static function called whenever whatever "createEventTrigger" spins up decides. 
// NOTE: I still don't think I have a complete understanding of how this eventing
// system works. I'm sure I will know more as I see it operate. 
void SimpleFramedSource::onEventTriggered(void* clientData)
{
    ((SimpleFramedSource*)clientData)->DeliverNALUnitsToLive555FromQueue();
}

// pops shit off the NAL queue and sends it to live555. 
void SimpleFramedSource::DeliverNALUnitsToLive555FromQueue()
{
    // this is basically a direct copy of the stuff from 
    // https://github.com/RafaelPalomar/H264LiveStreamer/blob/master/LiveSourceWithx264.cxx. 
    // i do not understand enough about the formatting issues to know or care about these details. 


    if (!isCurrentlyAwaitingData()) return;
    x264_nal_t nal = this->_nalQueue.front();
    this->_nalQueue.pop();
    assert(nal.p_payload != NULL);
    // You need to remove the start code which is there in front of every nal unit.
    // the start code might be 0x00000001 or 0x000001. so detect it and remove it. pass remaining data to live555
    int trancate = 0;
    if (nal.i_payload >= 4 && nal.p_payload[0] == 0 && nal.p_payload[1] == 0 && nal.p_payload[2] == 0 && nal.p_payload[3] == 1)
    {
        trancate = 4;
    }
    else
    {
        if (nal.i_payload >= 3 && nal.p_payload[0] == 0 && nal.p_payload[1] == 0 && nal.p_payload[2] == 1)
        {
            trancate = 3;
        }
    }

    if (nal.i_payload - trancate > fMaxSize)
    {
        fFrameSize = fMaxSize;
        fNumTruncatedBytes = nal.i_payload - trancate - fMaxSize;
    }
    else
    {
        fFrameSize = nal.i_payload - trancate;
    }
    fPresentationTime = this->_time;
    memmove(fTo, nal.p_payload + trancate, fFrameSize);
    FramedSource::afterGetting(this);
}

SimpleFramedSource::~SimpleFramedSource()
{
    if (this->_encoder != NULL)
    {
        // tear down the x264 encoder. 
        this->_encoder->UnInitilize(); 
        delete this->_encoder;
    }
}
