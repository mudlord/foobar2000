#ifndef		IIRFILTERS_H
#define		IIRFILTERS_H

/* filter types */
enum {
	LPF, /* low pass filter */
	HPF, /* High pass filter */
	BPCSGF,/* band pass filter 1 */
	BPZPGF,/* band pass filter 2 */
	APF, /* Allpass filter*/
	NOTCH, /* Notch Filter */
	RIAA_phono, /* RIAA record/tape deemphasis */
	PEQ, /* Peaking band EQ filter */
	BBOOST, /* Bassboost filter */
	LSH, /* Low shelf filter */
	RIAA_CD, /* CD de-emphasis */
	HSH /* High shelf filter */

};

class IIRFilter
{
private:                           
	float pf_freq, pf_qfact, pf_gain;
	int type, pf_q_is_bandwidth; 
	float xn1,xn2,yn1,yn2;
	float omega, cs, a1pha, beta, b0, b1, b2, a0, a1,a2, A, sn;
public:
	
	IIRFilter();
	~IIRFilter();
	float Process(float samp);
	void setFrequency(float val);
	void setQuality(float val);
	void setGain(float val);
	void init(int samplerate,int filter_type);
	void make_poly_from_roots(double const * roots, size_t num_roots, float * poly);
};


#endif		//ECHO_H
