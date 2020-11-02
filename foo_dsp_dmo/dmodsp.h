#ifndef		DMODSP_H
#define		DMODSP_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <malloc.h>
#include <mediaobj.h>
#include <dshow.h>
#include <uuids.h>
#include <medparam.h>

/* filter types */
enum DMO_Type {
	AEC = 0,
	CHORUS = 1, 
    COMPRESSOR = 2,
	DISTORTION = 3,
	ECHO =4,
	FLANGER=5, 
	GARGLE=6, 
	REVERB=7, 
	PEQ=8,
	REVERBWAVES=9,
	RESAMPLER=10
};

class DMOFilter
{
private:                           
	int sample_rate;
	int n_ch;
	HRESULT COMInit;
	IMediaObject *m_pObject;
	IMediaObjectInPlace *m_pMediaProcess;
	IMediaParamInfo *m_pParamInfo;
	IMediaParams *m_pMediaParams;
public:
	
	DMOFilter();
	~DMOFilter();
	void Process(float *samples, int numsamples);
	void deinit();
	void init(int samplerate,int filter_type,int num_channels);
	void setparameter(int index, float param);
};


#endif		//ECHO_H
