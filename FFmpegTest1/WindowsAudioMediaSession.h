#pragma once
#include "liveMedia.hh"
#include "OnDemandServerMediaSubsession.hh"
#include "WindowsAudioInputDevice_noMixer.hh"

class WindowsAudioMediaSession : public OnDemandServerMediaSubsession
{
public:
    static WindowsAudioMediaSession* createNew(UsageEnvironment& env, bool reuseFirstSource);
    void checkForAuxSDPLine1();
    void afterPlayingDummy1();
protected:
    WindowsAudioMediaSession(UsageEnvironment& env, bool reuseFirstSource);
    virtual ~WindowsAudioMediaSession(void);
    void setDoneFlag() { fDoneFlag = ~0; }
protected:
    virtual char const* getAuxSDPLine(RTPSink* rtpSink, FramedSource* inputSource);
    virtual FramedSource* createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate);
    virtual RTPSink* createNewRTPSink(Groupsock* rtpGroupsock, unsigned char rtpPayloadTypeIfDynamic, FramedSource* inputSource);
private:
    char* fAuxSDPLine;
    char fDoneFlag;
    RTPSink* fDummySink;
};

