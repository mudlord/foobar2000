#define _USE_MATH_DEFINES
#include "../SDK/foobar2000.h"
#include "dmodsp.h"
#include <atlbase.h>
#include <uuids.h>
#include <medparam.h>
#include <dmoreg.h>
#include <mmsystem.h>

const char* knownDMOs[] =
{
	"{745057C7-F353-4F2D-A7EE-58434477730E}", // AEC (Acoustic echo cancellation, not usable)
	"{EFE6629C-81F7-4281-BD91-C9D604A95AF6}", // Chorus
	"{EF011F79-4000-406D-87AF-BFFB3FC39D57}", // Compressor
	"{EF114C90-CD1D-484E-96E5-09CFAF912A21}", // Distortion
	"{EF3E932C-D40B-4F51-8CCF-3F98F1B29D5D}", // Echo
	"{EFCA3D92-DFD8-4672-A603-7420894BAD98}", // Flanger
	"{DAFD8210-5711-4B91-9FE3-F75B7AE279BF}", // Gargle
	"{EF985E71-D5C7-42D4-BA4D-2D073E2E96F4}", // I3DL2Reverb
	"{120CED89-3BF4-4173-A132-3CB406CF3231}", // ParamEq
	"{87FC0268-9A55-4360-95AA-004A1D9DE26C}", // WavesReverb
	"{F447B69E-1884-4A7E-8055-346F74D6EDB3}" // Resampler DMO (not usable)
	};

#ifndef M_PI
#define M_PI		3.1415926535897932384626433832795
#endif
#define sqr(a) ((a) * (a))

DMOFilter::DMOFilter()
{
	m_pMediaParams = nullptr;
	m_pParamInfo = nullptr;
	m_pMediaProcess = nullptr;
	m_pObject = nullptr;
}

DMOFilter::~DMOFilter()
{ 

}

void DMOFilter::deinit()
{
	if (m_pMediaParams)
	{
		m_pMediaParams->Release();
		m_pMediaParams = nullptr;
	}
	if (m_pParamInfo)
	{
		m_pParamInfo->Release();
		m_pParamInfo = nullptr;
	}
	if (m_pMediaProcess)
	{
		m_pMediaProcess->Release();
		m_pMediaProcess = nullptr;
	}
	if (m_pObject)
	{
		m_pObject->Release();
		m_pObject = nullptr;
	}
	CoUninitialize();
}

void DMOFilter::init(int samplerate, int filter_type, int num_channels)
{
	DMO_MEDIA_TYPE mt;
	WAVEFORMATEX wfx;
	CLSID clsid1;
	CLSIDFromString(CComBSTR(knownDMOs[filter_type]), &clsid1);
	CoInitializeEx(NULL, COINIT_MULTITHREADED);
	HRESULT hr = CoCreateInstance(clsid1,
		NULL,
		CLSCTX_INPROC_SERVER,
		IID_IMediaObject,
		(void **)&m_pObject);

	hr = m_pObject->QueryInterface(IID_IMediaObjectInPlace, (void**)&m_pMediaProcess);
	m_pObject->QueryInterface(IID_IMediaParamInfo, (void **)&m_pParamInfo);
	m_pObject->QueryInterface(IID_IMediaParams, (void **)&m_pMediaParams);

	mt.majortype = MEDIATYPE_Audio;
	mt.subtype = MEDIASUBTYPE_PCM;
	mt.bFixedSizeSamples = TRUE;
	mt.bTemporalCompression = FALSE;
	mt.formattype = FORMAT_WaveFormatEx;
	mt.pUnk = nullptr;
	mt.pbFormat = (LPBYTE)&wfx;
	mt.cbFormat = sizeof(WAVEFORMATEX);
	mt.lSampleSize = sizeof(float);
	wfx.wFormatTag = 3; // WAVE_FORMAT_IEEE_FLOAT;
	wfx.nChannels = num_channels;
	wfx.nSamplesPerSec = samplerate;
	wfx.wBitsPerSample = sizeof(float) * 8;
	wfx.nBlockAlign = wfx.nChannels * (wfx.wBitsPerSample / 8);
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;
	wfx.cbSize = 0;
	hr = m_pObject->SetInputType(0,    //Input Stream index
		&mt,
		0);  // No flags specified
	hr = m_pObject->SetOutputType(0,       // Output Stream Index
		&mt,
		0);  // No flags specified


	DWORD dwParamCount = 0;
	m_pParamInfo->GetParamCount(&dwParamCount);
	for (int i = 0;i<  dwParamCount;i++)
	{
		MP_PARAMINFO mpi;
		//mpi.mpType = MPT_INT;
		mpi.szUnitText[0] = 0;
		mpi.szLabel[0] = 0;
		

		if (m_pParamInfo->GetParamInfo(i, &mpi) == S_OK)
		{
			float fMin = mpi.mpdMinValue;
			float fMax = mpi.mpdMaxValue;
			float default = mpi.mpdNeutralValue;
			if (mpi.mpType == MPT_BOOL)
			{
				fMin = 0;
				fMax = 1;
			}
		}
	}
}

void DMOFilter::setparameter(int index, float fValue)
{
	MP_PARAMINFO mpi;
	memset(&mpi, 0, sizeof(MP_PARAMINFO));
	if (m_pParamInfo->GetParamInfo(index, &mpi) == S_OK)
	{
		float fMin = mpi.mpdMinValue;
		float fMax = mpi.mpdMaxValue;

		if (mpi.mpType == MPT_BOOL)
		{
			fMin = 0;
			fMax = 1;
			fValue = (fValue > 0.5f) ? 1.0f : 0.0f;
		}
		if (fMax > fMin) fValue *= (fMax - fMin);
		fValue += fMin;
		if (fValue < fMin) fValue = fMin;
		if (fValue > fMax) fValue = fMax;
		if (mpi.mpType != MPT_FLOAT) fValue = (float)(int)fValue;
		m_pMediaParams->SetParam(index, fValue);
	}
}



void DMOFilter::Process(float *samples , int numsamples)
{
	static const float _f2si = 32768.0f;
	static const float _si2f = 1.0f / 32768.0f;
	for (int i = 0; i < numsamples;i++)samples[i] *=_f2si;
	HRESULT res = m_pMediaProcess->Process(numsamples * sizeof(float), reinterpret_cast<BYTE *>(samples), 0, DMO_INPLACE_NORMAL);
	for (int i = 0; i < numsamples; i++)samples[i] *= _si2f;
}