#include "stdafx.h"
#include "WindowsAudioMediaSession.h"

WindowsAudioMediaSession* WindowsAudioMediaSession::createNew(UsageEnvironment& env, bool reuseFirstSource)
{
    return new WindowsAudioMediaSession(env, reuseFirstSource);
}

WindowsAudioMediaSession::WindowsAudioMediaSession(UsageEnvironment& env, bool reuseFirstSource) :OnDemandServerMediaSubsession(env, reuseFirstSource), fAuxSDPLine(NULL), fDoneFlag(0), fDummySink(NULL)
{

}


WindowsAudioMediaSession::~WindowsAudioMediaSession(void)
{
    delete[] fAuxSDPLine;
}


static void afterPlayingDummy(void* clientData)
{
    WindowsAudioMediaSession *session = (WindowsAudioMediaSession*)clientData;
    session->afterPlayingDummy1();
}

void WindowsAudioMediaSession::afterPlayingDummy1()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
    WindowsAudioMediaSession* session = (WindowsAudioMediaSession*)clientData;
    session->checkForAuxSDPLine1();
}

void WindowsAudioMediaSession::checkForAuxSDPLine1()
{
    char const* dasl;
    if (fAuxSDPLine != NULL)
    {
        setDoneFlag();
    }
    else if (fDummySink != NULL && (dasl = fDummySink->auxSDPLine()) != NULL)
    {
        fAuxSDPLine = strDup(dasl);
        fDummySink = NULL;
        setDoneFlag();
    }
    else
    {
        int uSecsDelay = 100000;
        nextTask() = envir().taskScheduler().scheduleDelayedTask(uSecsDelay, (TaskFunc*)checkForAuxSDPLine, this);
    }
}

char const* WindowsAudioMediaSession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
{
    if (fAuxSDPLine != NULL) return fAuxSDPLine;
    if (fDummySink == NULL)
    {
        fDummySink = rtpSink;
        fDummySink->startPlaying(*inputSource, afterPlayingDummy, this);
        checkForAuxSDPLine(this);
    }

    envir().taskScheduler().doEventLoop(&fDoneFlag);
    return fAuxSDPLine;
}

FramedSource* WindowsAudioMediaSession::createNewStreamSource(unsigned clientSessionID, unsigned& estBitRate)
{
    // Based on encoder configuration i kept it 90000
    estBitRate = 90000;
    int inputPortNumber = 0;
    unsigned char bitsPerSample = 16000;
    unsigned char numChannels = 1;
    unsigned samplingFrequency = 8000;
    unsigned granularityInMS = 20;
    AudioInputDevice *source = WindowsAudioInputDevice::createNew(envir(),
        inputPortNumber, bitsPerSample, numChannels, samplingFrequency, granularityInMS); 
    // are you trying to keep the reference of the source somewhere? you shouldn't.  
    // Live555 will create and delete this class object many times. if you store it somewhere  
    // you will get memory access violation. instead you should configure you source to always read from your data source
    return AC3AudioStreamFramer::createNew(envir(), source);
}

RTPSink* WindowsAudioMediaSession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
