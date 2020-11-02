#ifndef		PHASER_H
#define		PHASER_H

class Phaser
{
private:
   float freq;
   float startphase;
   float fb;
   int depth;
   int stages;
   int drywet;
   unsigned long skipcount;
   float old[24];
   float gain;
   float fbout;
   float lfoskip;
   float phase;
public:
	Phaser();
	~Phaser();
	float Process(float input);
	void init(int samplerate);
	void SetLFOFreq(float val);
	void SetLFOStartPhase(float val);
	void SetFeedback(float val);
	void SetDepth(int val);
	void SetStages(int val);
	void SetDryWet(int val);
};


#endif		//ECHO_H
