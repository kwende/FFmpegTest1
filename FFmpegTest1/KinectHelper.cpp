#include "stdafx.h"
#include "KinectHelper.h"

IKinectSensor*          KinectHelper::m_pKinectSensor;
IDepthFrameReader*      KinectHelper::m_pDepthFrameReader;
IInfraredFrameReader*   KinectHelper::m_pInfraredFrameReader;

USHORT* KinectHelper::GetIRBuffer()
{
    INT64 nTime = 0;
    IFrameDescription* pFrameDescription = NULL;
    int nWidth = 0;
    int nHeight = 0;
    USHORT nDepthMinReliableDistance = 0;
    USHORT nDepthMaxDistance = 0;
    UINT nBufferSize = 0;
    UINT16 *pBuffer = NULL;

    IInfraredFrame* pInfraredFrame = NULL;

    HRESULT hr = S_OK;
    do
    {
        hr = m_pInfraredFrameReader->AcquireLatestFrame(&pInfraredFrame);
    } while (FAILED(hr));

    hr = pInfraredFrame->get_RelativeTime(&nTime);

    if (SUCCEEDED(hr))
    {
        hr = pInfraredFrame->get_FrameDescription(&pFrameDescription);
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
        hr = pInfraredFrame->AccessUnderlyingBuffer(&nBufferSize, &pBuffer);
    }

    UINT16* irBuffer = new UINT16[nWidth * nHeight];

    for (int c = 0; c < nWidth * nHeight; c++)
    {
        // normalize the incoming infrared data (ushort) to a float ranging from 
        // [InfraredOutputValueMinimum, InfraredOutputValueMaximum] by
        // 1. dividing the incoming value by the source maximum value
        float intensityRatio = static_cast<float>(pBuffer[c]) / InfraredSourceValueMaximum;

        // 2. dividing by the (average scene value * standard deviations)
        intensityRatio /= InfraredSceneValueAverage * InfraredSceneStandardDeviations;

        // 3. limiting the value to InfraredOutputValueMaximum
        intensityRatio = min(InfraredOutputValueMaximum, intensityRatio);

        // 4. limiting the lower value InfraredOutputValueMinimym
        intensityRatio = max(InfraredOutputValueMinimum, intensityRatio);

        // 5. converting the normalized value to a byte and using the result
        // as the RGB components required by the image
        byte intensity = static_cast<byte>(intensityRatio * 255.0f);

        irBuffer[c] = intensity;
    }

    if (pInfraredFrame)
    {
        pInfraredFrame->Release();
    }

    if (pFrameDescription)
    {
        pFrameDescription->Release();
    }


    return irBuffer;
}

USHORT* KinectHelper::GetDepthBuffer()
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

    HRESULT hr = S_OK;
    do
    {
        hr = m_pDepthFrameReader->AcquireLatestFrame(&pDepthFrame);
    } while (FAILED(hr));

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

    UINT16* depthBuffer = new UINT16[nWidth * nHeight];

    for (int c = 0; c < nWidth * nHeight; c++)
    {
        depthBuffer[c] = pBuffer[c];
    }

    if (pDepthFrame)
    {
        pDepthFrame->Release();
    }

    if (pFrameDescription)
    {
        pFrameDescription->Release();
    }

    return depthBuffer;
}

void KinectHelper::Stop()
{
    m_pKinectSensor->Close();
    m_pKinectSensor = NULL;
}

void KinectHelper::Start()
{
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

            if (SUCCEEDED(hr))
            {
                hr = pDepthFrameSource->OpenReader(&m_pDepthFrameReader);
            }
        }

        IInfraredFrameSource* pInfraredFrameSource = NULL;

        if (SUCCEEDED(hr))
        {
            hr = m_pKinectSensor->get_InfraredFrameSource(&pInfraredFrameSource);

            if (SUCCEEDED(hr))
            {
                hr = pInfraredFrameSource->OpenReader(&m_pInfraredFrameReader);
            }
        }

        if (pDepthFrameSource != NULL)
        {
            pDepthFrameSource->Release();
        }

        if (pInfraredFrameSource != NULL)
        {
            pInfraredFrameSource->Release();
        }
    }
}
