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
    _lastTickCount = ::GetTickCount();
    m_pCachedBuffer = 0;
    m_fps = 15;

    HRESULT hr;

    hr = GetDefaultKinectSensor(&m_pKinectSensor);
    if (FAILED(hr))
    {
        return;
    }

    if (m_pKinectSensor)
    {
        // Initialize the Kinect and get the depth reader
        IDepthFrameSource* pDepthFrameSource = NULL;

        hr = m_pKinectSensor->Open();

        if (SUCCEEDED(hr))
        {
            hr = m_pKinectSensor->get_DepthFrameSource(&pDepthFrameSource);
        }

        if (SUCCEEDED(hr))
        {
            hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
        }

        //SafeRelease(pDepthFrameSource);
        if (pDepthFrameSource != NULL)
        {
            pDepthFrameSource->Release();
        }
    }

    gettimeofday(&_time, NULL);
    //_time.tv_sec = 0;
    //_time.tv_usec = 0;

    //this->_videoCap = new cv::VideoCapture("c:/users/brush/desktop/feynman.mp4");
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
    long currentTickCount = ::GetTickCount();

    std::cout << (currentTickCount - _lastTickCount) / 1000.0f << std::endl;

    _lastTickCount = currentTickCount;

    if (this->_nalQueue.empty())
    {
        // get a frame of data, encode, and enqueue it. 
        this->GetFrameAndEncodeToNALUnitsAndEnqueue();
        // get time of day for the broadcaster

        //long microseconds = _time.tv_usec;
        //microseconds += 33000;
        //long numberOfSeconds = microseconds / 1000000;
        //long remainingNumberOfMicroseconds = microseconds % 1000000;

        ////66000 microseconds = 66 milliseconds
        //_time.tv_sec += numberOfSeconds;
        //_time.tv_usec = remainingNumberOfMicroseconds;

        //std::cout << _time.tv_sec << "." << _time.tv_usec << std::endl; 
        ::gettimeofday(&_time, NULL);

        // take the nal units and push them to live 555. 
        this->DeliverNALUnitsToLive555FromQueue(true);
    }
    else
    {
        // there's already stuff to deliver, so just deliver it. 
        this->DeliverNALUnitsToLive555FromQueue(false);
    }
}

USHORT* SimpleFramedSource::GetBuffer()
{
    INT64 nTime = 0;
    IFrameDescription* pFrameDescription = NULL;
    int nWidth = 0;
    int nHeight = 0;
    USHORT nDepthMinReliableDistance = 0;
    USHORT nDepthMaxDistance = 0;
    UINT nBufferSize = 0;
    UINT16 *pBuffer = NULL;

    IDepthFrame* pDepthFrame = NULL;

    //HANDLE hEvents[] = { reinterpret_cast<HANDLE>(m_WaitHandle) };
    //WaitForMultipleObjects(1, hEvents, true, INFINITE);

    //IDepthFrameArrivedEventArgs* pDepthArgs = nullptr;
    //HRESULT hr = m_pDepthFrameReader->GetFrameArrivedEventData(m_WaitHandle, &pDepthArgs);
    HRESULT hr = S_OK;
    do
    {
        hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
    } while (FAILED(hr));

    //::Sleep(30); 

    //pDepthArgs->Release(); 

    hr = pDepthFrame->get_RelativeTime(&nTime);

    if (SUCCEEDED(hr))
    {
        hr = pDepthFrame->get_FrameDescription(&pFrameDescription);
    }

    if (SUCCEEDED(hr))
    {
        hr = pFrameDescription->get_Width(&nWidth);
    }

    if (SUCCEEDED(hr))
    {
        hr = pFrameDescription->get_Height(&nHeight);
    }

    if (SUCCEEDED(hr))
    {
        hr = pDepthFrame->get_DepthMinReliableDistance(&nDepthMinReliableDistance);
    }

    if (SUCCEEDED(hr))
    {
        // In order to see the full range of depth (including the less reliable far field depth)
        // we are setting nDepthMaxDistance to the extreme potential depth threshold
        nDepthMaxDistance = USHRT_MAX;

        // Note:  If you wish to filter by reliable depth distance, uncomment the following line.
        //// hr = pDepthFrame->get_DepthMaxReliableDistance(&nDepthMaxDistance);
    }

    if (SUCCEEDED(hr))
    {
        hr = pDepthFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
    }

    if (!m_pCachedBuffer)
    {
        m_pCachedBuffer = new UINT16[nWidth * nHeight];
    }

    for (int c = 0; c < nWidth * nHeight; c++)
    {
        m_pCachedBuffer[c] = pBuffer[c];
    }

    if (pDepthFrame)
    {
        pDepthFrame->Release();
    }

    if (pFrameDescription)
    {
        pFrameDescription->Release();
    }
    //else
    //{
    //    std::cout << "-";
    //}

    return m_pCachedBuffer;
}

// this is the guy which will take data, encode it, and push it to the NAL queue. 
void SimpleFramedSource::GetFrameAndEncodeToNALUnitsAndEnqueue()
{
    int skipLength = 1;

    int nHeight = 424, nWidth = 512;

    USHORT* pBuffer = GetBuffer();

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

    //delete pBuffer;
}

// Static function called whenever whatever "createEventTrigger" spins up decides. 
// NOTE: I still don't think I have a complete understanding of how this eventing
// system works. I'm sure I will know more as I see it operate. 
void SimpleFramedSource::onEventTriggered(void* clientData)
{
    ((SimpleFramedSource*)clientData)->DeliverNALUnitsToLive555FromQueue(false);
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
    //std::cout << (this->_time.tv_sec - lastPresentationTime.tv_sec) << std::endl;
    //std::cout << this->_time.tv_sec << "." << this->_time.tv_usec << std::endl;
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

    if (m_pKinectSensor)
    {
        m_pKinectSensor->Close();
        m_pKinectSensor = NULL;
    }
}
