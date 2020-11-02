#ifndef FREEVERB_H
#define FREEVERB_H

// FIXME: Fix this really ugly hack
inline float undenormalise(void *sample) {
	if (((*(unsigned int*)sample) &  0x7f800000) == 0)
		return 0.0f;
	return *(float*)sample;
}


class comb {
public:
	comb();
	void setbuffer(float *buf, int size);
	inline float process(float inp);
	void mute();
	void setdamp(float val);
	float getdamp();
	void setfeedback(float val);
	float getfeedback();
private:
	float feedback;
	float filterstore;
	float damp1;
	float damp2;
	float *buffer;
	int bufsize;
	int bufidx;
};

inline float comb::process(float input) {
	float output;

	output = buffer[bufidx];
	undenormalise(&output);

	filterstore = (output * damp2) + (filterstore * damp1);
	undenormalise(&filterstore);

	buffer[bufidx] = input + (filterstore * feedback);

	if (++bufidx >= bufsize)
		bufidx = 0;

	return output;
}

class allpass {
public:
	allpass();
	void setbuffer(float *buf, int size);
	inline float process(float inp);
	void mute();
	void setfeedback(float val);
	float getfeedback();
private:
	float feedback;
	float *buffer;
	int bufsize;
	int bufidx;
};

inline float allpass::process(float input) {
	float output;
	float bufout;

	bufout = buffer[bufidx];
	undenormalise(&bufout);

	output = -input + bufout;
	buffer[bufidx] = input + (bufout * feedback);

	if (++bufidx >= bufsize)
		bufidx = 0;

	return output;
}


const int	numcombs	= 8;
const int	numallpasses	= 4;
const float	muted		= 0;
const float	fixedgain	= 0.015f;
const float	scalewet	= 3;
const float	scaledry	= 2;
const float	scaledamp	= 0.4f;
const float	scaleroom	= 0.28f;
const float	offsetroom	= 0.7f;
const float	initialroom	= 0.5f;
const float	initialdamp	= 0.5f;
const float	initialwet	= 1 / scalewet;
const float	initialdry	= 0;
const float	initialwidth	= 1;
const float	initialmode	= 0;
const float	freezemode	= 0.5f;

const int num_comb = 8;
const int num_allpass = 4;


class revmodel {
public:
	revmodel();
	~revmodel();
	void mute();
	void init(int srate);
	float revmodel::processsample(float in);
	void setroomsize(float value);
	float getroomsize();
	void setdamp(float value);
	float getdamp();
	void setwet(float value);
	float getwet();
	void setdry(float value);
	float getdry();
	void setwidth(float value);
	float getwidth();
	void setmode(float value);
	float getmode();
private:
	void update();

	float gain;
	float roomsize, roomsize1;
	float damp, damp1;
	float wet, wet1, wet2;
	float dry;
	float width;
	float mode;

	comb combL[numcombs];


	allpass	allpassL[numallpasses];


	float **bufcomb;
	float **bufallpass;
};

#endif
