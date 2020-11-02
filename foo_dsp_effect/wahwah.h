#ifndef		WAHWAH_H
#define		WAHWAH_H

class WahWah
{
private:
   float phase;
   float lfoskip;
   unsigned long skipcount;
   float xn1, xn2, yn1, yn2;
   float b0, b1, b2, a0, a1, a2;
   float freq, startphase;
   float depth, freqofs, res;
public:   
   WahWah();
   ~WahWah();
	float Process(float samp);
	void init(int samplerate);
	void SetLFOFreq(float val);
	void SetLFOStartPhase(float val);
	void SetDepth(float val);
	void SetFreqOffset(float val);
	void SetResonance(float val);
};


#endif		//ECHO_H
