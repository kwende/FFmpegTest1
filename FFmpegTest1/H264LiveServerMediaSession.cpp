#include "stdafx.h"
#include "H264LiveServerMediaSession.h"

H264LiveServerMediaSession* H264LiveServerMediaSession::createNew(UsageEnvironment& env, bool reuseFirstSource)
{
    return new H264LiveServerMediaSession(env, reuseFirstSource);
}

H264LiveServerMediaSession::H264LiveServerMediaSession(UsageEnvironment& env, bool reuseFirstSource) :
    OnDemandServerMediaSubsession(env, reuseFirstSource), fAuxSDPLine(NULL), fDoneFlag(0), fDummySink(NULL)
{
    KinectHelper::Start(); 
}


H264LiveServerMediaSession::~H264LiveServerMediaSession(void)
{
    delete[] fAuxSDPLine;
    KinectHelper::Stop();
}


static void afterPlayingDummy(void* clientData)
{
    H264LiveServerMediaSession *session = (H264LiveServerMediaSession*)clientData;
    session->afterPlayingDummy1();
}

void H264LiveServerMediaSession::afterPlayingDummy1()
{
    envir().taskScheduler().unscheduleDelayedTask(nextTask());
    setDoneFlag();
}

static void checkForAuxSDPLine(void* clientData)
{
    H264LiveServerMediaSession* session = (H264LiveServerMediaSession*)clientData;
    session->checkForAuxSDPLine1();
}

void H264LiveServerMediaSession::checkForAuxSDPLine1()
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

char const* H264LiveServerMediaSession::getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource)
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

FramedSource* H264LiveServerMediaSession::createNewStreamSource(unsigned clientSessionID, unsigned& estBitRate)
{
    estBitRate = 90000;
    SimpleFramedSource *source = SimpleFramedSource::createNew(envir());
    return H264VideoStreamDiscreteFramer::createNew(envir(), source);
}

RTPSink* H264LiveServerMediaSession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    return H264VideoRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic);
}
