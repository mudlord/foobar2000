#ifndef		ECHO_H
#define		ECHO_H

// A simple echo (or delay) class. Stores just enough history data
// and mixes it with the input. History data is amplified with values
// less than 1 to produce echoes.
class Echo
{
private:
	float *history; // history buffer
	int pos;        // current position in history buffer
	int amp;        // amplification of echoes (0-256)
	int delay;      // delay in number of samples
	int ms;         // delay in miliseconds
	int rate;       // sample rate
	int feedback;   // feedback

	float f_amp;    // amplification (0-1)
	float f_feedback; //feedback

public:
	Echo();
	~Echo();

	void SetDelay(int ms);
	void SetAmp(int amp);
	void SetSampleRate(int rate);
	void SetFeedback(int feedback);
	inline int GetFeedback() const;
	inline int GetDelay() const;
	inline int GetAmp() const;
	inline int GetSampleRate() const;
	float Process(float in);
};


#endif		//ECHO_H
