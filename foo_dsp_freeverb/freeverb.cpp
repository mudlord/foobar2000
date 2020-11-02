

// Comb filter implementation
//
// Written by
// http://www.dreampoint.co.uk
// This code is public domain

#include "freeverb.h"

comb::comb() {
	filterstore = 0;
	bufidx = 0;
}

void comb::setbuffer(float *buf, int size) {
	buffer = buf;
	bufsize = size;
}

void comb::mute() {
	for (int i = 0; i < bufsize; i++)
		buffer[i] = 0;
}

void comb::setdamp(float val) {
	damp1 = val;
	damp2 = 1 - val;
}

float comb::getdamp() {
	return damp1;
}

void comb::setfeedback(float val) {
	feedback = val;
}

float comb::getfeedback() {
	return feedback;
}

// Allpass filter implementation

allpass::allpass() {
	bufidx = 0;
}

void allpass::setbuffer(float *buf, int size) {
	buffer = buf;
	bufsize = size;
}

void allpass::mute() {
	for (int i = 0; i < bufsize; i++)
		buffer[i] = 0;
}

void allpass::setfeedback(float val) {
	feedback = val;
}

float allpass::getfeedback() {
	return feedback;
}

// Reverb model implementation

revmodel::revmodel() {
	// Tie the components to their buffers
	combL[0].setbuffer(bufcombL1,combtuningL1);
	combR[0].setbuffer(bufcombR1,combtuningR1);
	combL[1].setbuffer(bufcombL2,combtuningL2);
	combR[1].setbuffer(bufcombR2,combtuningR2);
	combL[2].setbuffer(bufcombL3,combtuningL3);
	combR[2].setbuffer(bufcombR3,combtuningR3);
	combL[3].setbuffer(bufcombL4,combtuningL4);
	combR[3].setbuffer(bufcombR4,combtuningR4);
	combL[4].setbuffer(bufcombL5,combtuningL5);
	combR[4].setbuffer(bufcombR5,combtuningR5);
	combL[5].setbuffer(bufcombL6,combtuningL6);
	combR[5].setbuffer(bufcombR6,combtuningR6);
	combL[6].setbuffer(bufcombL7,combtuningL7);
	combR[6].setbuffer(bufcombR7,combtuningR7);
	combL[7].setbuffer(bufcombL8,combtuningL8);
	combR[7].setbuffer(bufcombR8,combtuningR8);
	allpassL[0].setbuffer(bufallpassL1,allpasstuningL1);
	allpassR[0].setbuffer(bufallpassR1,allpasstuningR1);
	allpassL[1].setbuffer(bufallpassL2,allpasstuningL2);
	allpassR[1].setbuffer(bufallpassR2,allpasstuningR2);
	allpassL[2].setbuffer(bufallpassL3,allpasstuningL3);
	allpassR[2].setbuffer(bufallpassR3,allpasstuningR3);
	allpassL[3].setbuffer(bufallpassL4,allpasstuningL4);
	allpassR[3].setbuffer(bufallpassR4,allpasstuningR4);

	// Set default values
	allpassL[0].setfeedback(0.5f);
	allpassR[0].setfeedback(0.5f);
	allpassL[1].setfeedback(0.5f);
	allpassR[1].setfeedback(0.5f);
	allpassL[2].setfeedback(0.5f);
	allpassR[2].setfeedback(0.5f);
	allpassL[3].setfeedback(0.5f);
	allpassR[3].setfeedback(0.5f);
	setwet(initialwet);
	setroomsize(initialroom);
	setdry(initialdry);
	setdamp(initialdamp);
	setwidth(initialwidth);
	setmode(initialmode);

	// Buffer will be full of rubbish - so we MUST mute them
	mute();
}

void revmodel::mute() {
	int i;

	if (getmode() >= freezemode)
		return;

	for (i = 0; i < numcombs; i++) {
		combL[i].mute();
		combR[i].mute();
	}

	for (i = 0; i < numallpasses; i++) {
		allpassL[i].mute();
		allpassR[i].mute();
	}
}

void revmodel::processmono(float *inout, unsigned int numsamples)
{
	float *in_ptr  = inout;
	float *out_ptr = inout;
	for(int i=0; i<numsamples; i++)
	{
		float mono_out = 0.0f;
		float mono_in = *in_ptr;
		float input = (mono_in) * gain;

		for(int i=0; i<numcombs; i++)
		{
			mono_out += combL[i].process(input);
		}

		for(int i=0; i<numallpasses; i++)
		{
			mono_out = allpassL[i].process(mono_out);
		}

		in_ptr++;
		*out_ptr = mono_in * dry + mono_out * wet1;
		out_ptr++;
	}
}

void revmodel::processstereo(float *inout, unsigned int numsamples)
{
	float *in_ptr  = inout;
	float *out_ptr = inout;
	for(int i=0; i<numsamples; i++)
	{
		float left_out = 0.0f;
		float left_in  = *in_ptr; //left chan
		float right_out = 0.0f;
		float right_in = *(in_ptr+1); //right chan
		float input = (left_in + right_in) * gain;
		for(int i=0; i<numcombs; i++)
		{
			left_out += combL[i].process(input);
			right_out += combR[i].process(input);
		}
		for(int i=0; i<numallpasses; i++)
		{
			left_out = allpassL[i].process(left_out);
			right_out = allpassR[i].process(right_out);
		}
		in_ptr+=2; //increment input pointers
		*out_ptr = left_in * dry + left_out * wet1;
		out_ptr++;
		*out_ptr = right_in * dry + right_out * wet1;
		out_ptr++;
	}
}


void revmodel::processreplace(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip) {
	float outL, outR, input;

	while (numsamples-- > 0) {
		int i;

		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for (i = 0; i < numcombs; i++) {
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for (i = 0; i < numallpasses; i++) {
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output REPLACING anything already there
		*outputL = outL * wet1 + outR * wet2 + *inputL * dry;
		*outputR = outR * wet1 + outL * wet2 + *inputR * dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::processmix(float *inputL, float *inputR, float *outputL, float *outputR, long numsamples, int skip) {
	float outL, outR, input;

	while (numsamples-- > 0) {
		int i;

		outL = outR = 0;
		input = (*inputL + *inputR) * gain;

		// Accumulate comb filters in parallel
		for (i = 0; i < numcombs; i++) {
			outL += combL[i].process(input);
			outR += combR[i].process(input);
		}

		// Feed through allpasses in series
		for (i = 0; i < numallpasses; i++) {
			outL = allpassL[i].process(outL);
			outR = allpassR[i].process(outR);
		}

		// Calculate output MIXING with anything already there
		*outputL += outL * wet1 + outR * wet2 + *inputL * dry;
		*outputR += outR * wet1 + outL * wet2 + *inputR * dry;

		// Increment sample pointers, allowing for interleave (if any)
		inputL += skip;
		inputR += skip;
		outputL += skip;
		outputR += skip;
	}
}

void revmodel::update() {
	// Recalculate internal values after parameter change

	int i;

	wet1 = wet * (width / 2 + 0.5f);
	wet2 = wet * ((1 - width) / 2);

	if (mode >= freezemode) {
		roomsize1 = 1;
		damp1 = 0;
		gain = muted;
	} else {
		roomsize1 = roomsize;
		damp1 = damp;
		gain = fixedgain;
	}

	for (i = 0; i < numcombs; i++) {
		combL[i].setfeedback(roomsize1);
		combR[i].setfeedback(roomsize1);
	}

	for (i = 0; i < numcombs; i++) {
		combL[i].setdamp(damp1);
		combR[i].setdamp(damp1);
	}
}

// The following get/set functions are not inlined, because
// speed is never an issue when calling them, and also
// because as you develop the reverb model, you may
// wish to take dynamic action when they are called.

void revmodel::setroomsize(float value) {
	roomsize = (value * scaleroom) + offsetroom;
	update();
}

float revmodel::getroomsize() {
	return (roomsize - offsetroom) / scaleroom;
}

void revmodel::setdamp(float value) {
	damp = value * scaledamp;
	update();
}

float revmodel::getdamp() {
	return damp / scaledamp;
}

void revmodel::setwet(float value) {
	wet = value * scalewet;
	update();
}

float revmodel::getwet() {
	return wet / scalewet;
}

void revmodel::setdry(float value) {
	dry = value * scaledry;
}

float revmodel::getdry() {
	return dry / scaledry;
}

void revmodel::setwidth(float value) {
	width = value;
	update();
}

float revmodel::getwidth() {
	return width;
}

void revmodel::setmode(float value) {
	mode = value;
	update();
}

float revmodel::getmode() {
	if (mode >= freezemode)
		return 1;
	else
		return 0;
}
