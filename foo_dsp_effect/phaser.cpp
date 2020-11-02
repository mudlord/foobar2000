#define _USE_MATH_DEFINES
#include "Phaser.h"

#include "../SDK/foobar2000.h"

#define phaserlfoshape 4.0
#define lfoskipsamples 20

Phaser::Phaser()
{
	
}


Phaser::~Phaser()
{ 
	for (int j = 0; j < stages; j++)
		old[j] = 0;   
}

void Phaser::SetLFOFreq(float val)
{
	freq = val;
}

void Phaser::SetLFOStartPhase(float val)
{
	startphase = val;
}

void Phaser::SetFeedback(float val)
{
	fb = val;
}

void Phaser::SetDepth(int val)
{
	depth = val;
}

void Phaser::SetStages(int val)
{
	stages = val;
}

void Phaser::SetDryWet(int val)
{
	drywet = val;
}


void Phaser::init(int samplerate)
{
	skipcount = 0;
	gain = 0.0;
	fbout = 0.0;
	lfoskip = freq * 2 * M_PI / samplerate;
	phase = startphase * M_PI / 180;
	for (int j = 0; j < stages; j++)
		old[j] = 0;   
}

float Phaser::Process(float in)
{
	float m, tmp, out;
	int i, j;
	m = in + fbout * fb / 100;
	if (((skipcount++) % lfoskipsamples) == 0) {
	gain = (1 + cos(skipcount * lfoskip + phase)) / 2;
	gain =(exp(gain * phaserlfoshape) - 1) / (exp(phaserlfoshape)-1);
    gain = 1 - gain / 255 * depth;  
	}
	for (j = 0; j < stages; j++) {
	tmp = old[j];
	old[j] = gain * tmp + m;
	m = tmp - gain * old[j];
	}
	fbout = m;
	out = (m * drywet + in * (255 - drywet)) / 255;
	return out;
}