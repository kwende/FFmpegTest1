#include "stdafx.h"
#include "SimpleFramedSource.h"

EventTriggerId SimpleFramedSource::serverDataReadyEventId = 0;
timeval SimpleFramedSource::_time;

DWORD WINAPI ReadThread(LPVOID lpParam)
{
    SimpleFramedSource* self = (SimpleFramedSource*)lpParam;
    self->pumpFrames();

    return 0;
}


SimpleFramedSource* SimpleFramedSource::createNew(UsageEnvironment& env)
{
    return new SimpleFramedSource(env);
}

bool SimpleFramedSource::Running()
{
    return this->_doThread;
}

timeval SimpleFramedSource::GetLatestTimeVal()
{
    return _time;
}

SimpleFramedSource::SimpleFramedSource(UsageEnvironment& env)
    :FramedSource(env)
{
    m_fps = 30;

    gettimeofday(&_time, NULL);

    this->_encoder = new x264Encoder();
    this->_encoder->Initilize();
    if (serverDataReadyEventId == 0)
    {
        serverDataReadyEventId = envir().taskScheduler().createEventTrigger(onEventTriggered);
    }

    ::InitializeCriticalSection(&_section);

    _serverNeedDataEvent = ::CreateEvent(
        NULL, FALSE,
        FALSE, TEXT("Event"));

    _serverShutDownEvent = ::CreateEvent(
        NULL, FALSE,
        FALSE, TEXT("SERVERDOWN")); 

    _doThread = true;
    DWORD dwThreadId;
    _thread = ::CreateThread(
        NULL, 0,
        ReadThread,
        this, 0, &dwThreadId);
}


void SimpleFramedSource::onEventTriggered(void* clientData)
{
    SimpleFramedSource* self = (SimpleFramedSource*)clientData;
    // if there is, pop off the frameQueue and encode
    self->GetFrameAndEncodeToNALUnitsAndEnqueue();

    // let's keep this here for now. it might not matter. 
    ::gettimeofday(&_time, NULL);

    self->DeliverNALUnitsToLive555FromQueue(true);
}

void SimpleFramedSource::pumpFrames()
{
    while(_doThread)
    {
        ::WaitForSingleObject(_serverNeedDataEvent, INFINITE);

        if (!_doThread)
        {
            break; 
        }

        // get kinect data. 
        USHORT* latestFrameBuffer = KinectHelper::GetIRBuffer();

        // synchronized set of data
        ::EnterCriticalSection(&_section);
        if (_latestFrameData)
        {
            delete _latestFrameData;
        }
        _latestFrameData = latestFrameBuffer;
        ::LeaveCriticalSection(&_section);

        // wait for the server to respond that it's processed the data and needs more. 
        envir().taskScheduler().triggerEvent(serverDataReadyEventId, this);
    }

    ::SetEvent(_serverShutDownEvent); 
}

void SimpleFramedSource::EncodeAndDeliverFrameData()
{
    bool dataFoundToDeliver = false;

    // is there anything in the NAL queue? If not, then 
    // see if there's anything new in the _frameQueue to encode.
    //if (this->_nalQueue.empty())
    {
        // if there is, pop off the frameQueue and encode
        this->GetFrameAndEncodeToNALUnitsAndEnqueue();

        // let's keep this here for now. it might not matter. 
        ::gettimeofday(&_time, NULL);

        // take the encoded frame nal units and push them to live 555. 
        this->DeliverNALUnitsToLive555FromQueue(true);
    }
    //else if (!this->_nalQueue.empty()) // the queue isn't empty. 
    //{
    //    // deliver the remaining stuff off the queue.  
    //    this->DeliverNALUnitsToLive555FromQueue(false);
    //}
}

void SimpleFramedSource::doStopGettingFrames()
{
    return;
}

void SimpleFramedSource::doGetNextFrame()
{
    DWORD start = ::GetTickCount(); 
    // do nothing. we're totally event driven. 
    if (!this->_nalQueue.empty()) // the queue isn't empty. 
    {
        // deliver the remaining stuff off the queue.  
        this->DeliverNALUnitsToLive555FromQueue(false);
    }
    else
    {
        ::SetEvent(_serverNeedDataEvent);
    }
}

// this is the guy which will take data, encode it, and push it to the NAL queue. 
void SimpleFramedSource::GetFrameAndEncodeToNALUnitsAndEnqueue()
{
    int skipLength = 2;

    int nHeight = 424, nWidth = 512;

    // USHORT* pBuffer = GetDepthBuffer();
    USHORT* pBuffer = new USHORT[512 * 424];

    ::EnterCriticalSection(&_section);
    for (int c = 0; c < 512 * 424; c++)
    {
        pBuffer[c] = _latestFrameData[c];
    }
    ::LeaveCriticalSection(&_section);

    DWORD dwStart = ::GetTickCount();


    // To convert to a byte, we're discarding the most-significant
    // rather than least-significant bits.
    // We're preserving detail, although the intensity will "wrap."
    // Values outside the reliable depth range are mapped to 0 (black).

    // Note: Using conditionals in this loop could degrade performance.
    // Consider using a lookup table instead when writing production code.
    //const UINT16* pBufferEnd = pBuffer + (nWidth * nHeight);

    cv::Mat rgbFrame = cv::Mat(nHeight / skipLength, nWidth / skipLength, CV_8UC3);

    for (int y = 0, rgbIndex = 0; y < nHeight; y += skipLength)
    {
        for (int x = 0; x < nWidth; x += skipLength, rgbIndex++)
        {
            USHORT depth = pBuffer[y * nWidth + x];

            // To convert to a byte, we're discarding the most-significant
            // rather than least-significant bits.
            // We're preserving detail, although the intensity will "wrap."
            // Values outside the reliable depth range are mapped to 0 (black).

            // Note: Using conditionals in this loop could degrade performance.
            // Consider using a lookup table instead when writing production code.
            BYTE intensity = static_cast<BYTE>((depth % 256));

            rgbFrame.data[(rgbIndex * 3)] = intensity;
            rgbFrame.data[(rgbIndex * 3) + 1] = intensity;
            rgbFrame.data[(rgbIndex * 3) + 2] = intensity;
        }
    }

    //cv::imwrite("c:/users/brush/desktop/output.jpg", rgbFrame);

    this->_encoder->EncodeFrame(rgbFrame);
    // now we dequeue the NAL units and put them onto our own queue. 
    while (this->_encoder->IsNalsAvailableInOutputQueue())
    {
        x264_nal_t nal = this->_encoder->getNalUnit();
        this->_nalQueue.push(nal);
    }

    delete pBuffer;
}

bool SimpleFramedSource::isCurrentlyAwaitingData()
{
    return this->_nalQueue.size() > 0;
}

// pops shit off the NAL queue and sends it to live555. 
void SimpleFramedSource::DeliverNALUnitsToLive555FromQueue(bool newData)
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
    //http://comments.gmane.org/gmane.comp.multimedia.live555.devel/4930
    //http://stackoverflow.com/questions/13863673/how-to-write-a-live555-framedsource-to-allow-me-to-stream-h-264-live
    timeval lastPresentationTime = fPresentationTime;
    fPresentationTime = this->_time;
    if (newData)
    {
        fDurationInMicroseconds = 1000000 / m_fps; // 66000;
    }
    else
    {
        fDurationInMicroseconds = 0;
    }
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

    _doThread = false;
    ::SetEvent(_serverNeedDataEvent);

    ::WaitForSingleObject(_serverShutDownEvent, INFINITE); 
    
    ::CloseHandle(_thread);
    ::CloseHandle(_serverNeedDataEvent);
    ::DeleteCriticalSection(&_section);
}
