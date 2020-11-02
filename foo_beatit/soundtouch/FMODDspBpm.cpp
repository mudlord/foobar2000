#ifndef _FMODDSPBPM_CPP_
#define _FMODDSPBPM_CPP_

#include <string.h>
#include "FMODDspBpm.h"
#include <BPMDetect.h>

#define BUFF_SIZE   1024

using namespace soundtouch;

FMODDspBpm::FMODDspBpm(FMODSystem * system, FMODSound * sound) : FMODDsp()
{
    this->sound = sound;
    this->bpmdetect = new BPMDetect(this->sound->getChannels(), (int)this->sound->getFrequency());

    FMOD_DSP_DESCRIPTION description;
    memset(&description, 0, sizeof(description));
    strcpy(description.name, "BPM Detection DSP\0");
    description.read = dspCallback;
    description.channels = 0;
    description.userdata = (void *)this->bpmdetect;

    FMODError::eval(FMOD_System_CreateDSP(system->getSystem(), &description, &this->fmod_dsp));

    this->sound->addDsp(this);
}

FMODDspBpm::~FMODDspBpm()
{
    delete this->bpmdetect;
}

float FMODDspBpm::getBpmSoFar()
{
    return this->bpmdetect->getBpm();
}

float FMODDspBpm::getBpm()
{
    int channels = this->sound->getChannels();
    int numBytes = sizeof(SAMPLETYPE);

    SAMPLETYPE samples[BUFF_SIZE];
    char * buffer = new char[BUFF_SIZE * numBytes];
    unsigned int bufferSizeInBytes = BUFF_SIZE * numBytes;

    BPMDetect bpm(channels, (int)this->sound->getFrequency());

    bool done = false;

    unsigned int read = 0;
    unsigned int readTotal = 0;

    while(!done)
    {
        read = this->sound->readData(buffer, bufferSizeInBytes);

        for(unsigned int i = 0; i < bufferSizeInBytes; i+= numBytes)
        {
			unsigned int index = i / numBytes;
			SAMPLETYPE sample = ((buffer[i + 3] & 0xFF) << 24) | ((buffer[i + 2] & 0xFF) << 16) | ((buffer[i + 1] & 0xFF) << 8) | (buffer[i] & 0xFF);
			samples[index] = sample;
        }

        if(read < BUFF_SIZE * sizeof(SAMPLETYPE))
        {
            done = true;
        }

        unsigned int numsamples = read / sizeof(SAMPLETYPE);

        bpm.inputSamples(samples, numsamples / channels);

        readTotal += read;
    }

    this->sound->seekData(0);

    delete buffer;

    return bpm.getBpm();
}

FMOD_RESULT F_CALLBACK dspCallback(FMOD_DSP_STATE *dsp_state, float *inbuffer, float *outbuffer, unsigned int length, int inchannels, int outchannels)
{
    BPMDetect * bpmdetect;
    FMOD_DSP * thisdsp = dsp_state->instance;
    FMOD_DSP_GetUserData(thisdsp, (void **)&bpmdetect);

    /// SAMPLETYPE is 'short' which is 4 bytes long
    /// so is 'float' and that's why everything is peachy.
    bpmdetect->inputSamples(inbuffer, length);

    for (unsigned int count = 0; count < length; count++)
    {
        for (int count2 = 0; count2 < outchannels; count2++)
        {
            /*
            short data = (short)inbuffer[(count * inchannels) + count2];
            if(data != 0)
            {
				printf("Data: %d\n", data);
            }
            */
            outbuffer[(count * outchannels) + count2] = inbuffer[(count * inchannels) + count2];
		}
    }

    return FMOD_OK;
}

#endif // _FMODDSPBPM_CPP_
