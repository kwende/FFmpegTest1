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
    return NULL;
    //if (fAuxSDPLine != NULL) return fAuxSDPLine;
    //if (fDummySink == NULL)
    //{
    //    fDummySink = rtpSink;
    //    fDummySink->startPlaying(*inputSource, afterPlayingDummy, this);
    //    checkForAuxSDPLine(this);
    //}

    //envir().taskScheduler().doEventLoop(&fDoneFlag);
    //return fAuxSDPLine;
}

FramedSource* WindowsAudioMediaSession::createNewStreamSource(unsigned clientSessionID, unsigned& estBitRate)
{
    // Based on encoder configuration i kept it 90000
    estBitRate = 90000;
    unsigned char bitsPerSample = 16;
    unsigned char numChannels = 1;
    unsigned samplingFrequency = 44100;
    unsigned granularityInMS = 20;
    //AudioInputDevice *source = WindowsAudioInputDevice::createNew(envir(),
    //    inputPortNumber, bitsPerSample, numChannels, samplingFrequency);

    AudioPortNames* names = AudioInputDevice::getPortNames();

    int i = 0;
    for (; i < names->numPorts; i++)
    {
        std::string portName(names->portName[i]);
        if (portName.find("tek") != std::string::npos)
        {
            break;
        }
    }

    AudioInputDevice *source = AudioInputDevice::createNew(envir(), i, bitsPerSample, numChannels, samplingFrequency);
    return EndianSwap16::createNew(envir(), source);
}

RTPSink* WindowsAudioMediaSession::createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource)
{
    int payloadFormatCode = 11;
    const char* mimeType = "L16";
    int fSamplingFrequency = 44100;
    int fNumChannels = 1;

    //return SimpleRTPSink::createNew(envir(), rtpGroupsock, rtpPayloadTypeIfDynamic, );
    SimpleRTPSink* sink = SimpleRTPSink::createNew(envir(), rtpGroupsock,
        rtpPayloadTypeIfDynamic, fSamplingFrequency,
        "audio", mimeType, fNumChannels);

    return sink;
}
