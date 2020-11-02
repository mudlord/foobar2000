#define _USE_MATH_DEFINES
#include "wahwah.h"

#include "../SDK/foobar2000.h"
#define lfoskipsamples 30


WahWah::WahWah()
{
}


WahWah::~WahWah()
{ 	
}

void WahWah::SetLFOFreq(float val)
{
	freq = val;
}

void WahWah::SetLFOStartPhase(float val)
{
	startphase = val;
}

void WahWah::SetDepth(float val)
{
	depth = val;
}

void WahWah::SetFreqOffset(float val)
{
	freqofs = val;
}

void WahWah::SetResonance(float val)
{
	res = val;
}


void WahWah::init(int samplerate)
{
	lfoskip = freq * 2 * M_PI / samplerate;
	skipcount = 0;
	xn1 = 0;
	xn2 = 0;
	yn1 = 0;
	yn2 = 0;
	b0 = 0;
	b1 = 0;
	b2 = 0;
	a0 = 0;
	a1 = 0;
	a2 = 0;
	phase = startphase * M_PI / 180;
}

float WahWah::Process(float samp)
{
	float frequency, omega, sn, cs, alpha;
	float in, out;
    in = samp;
	if ((skipcount++) % lfoskipsamples == 0) {
	frequency = (1 + cos(skipcount * lfoskip + phase)) / 2;
	frequency = frequency * depth * (1 - freqofs) + freqofs;
	frequency = exp((frequency - 1) * 6);
	omega = M_PI * frequency;
	sn = sin(omega);
	cs = cos(omega);
	alpha = sn / (2 * res);
	b0 = (1 - cs) / 2;
	b1 = 1 - cs;
	b2 = (1 - cs) / 2;
	a0 = 1 + alpha;
	a1 = -2 * cs;
	a2 = 1 - alpha;
	};
	out = (b0 * in + b1 * xn1 + b2 * xn2 - a1 * yn1 - a2 * yn2) / a0;
	xn2 = xn1;
	xn1 = in;
	yn2 = yn1;
	yn1 = out;
	samp = out;
	return samp;
}