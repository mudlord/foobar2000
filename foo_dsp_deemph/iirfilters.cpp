#define _USE_MATH_DEFINES
#include "iirfilters.h"

#include "../SDK/foobar2000.h"


#ifndef M_PI
#define M_PI		3.1415926535897932384626433832795
#endif
#define sqr(a) ((a) * (a))

IIRFilter::IIRFilter()
{
}

IIRFilter::~IIRFilter()
{ 
}

void IIRFilter::setFrequency(float val)
{
	pf_freq = val;
}
void IIRFilter::setQuality(float val)
{
	pf_qfact = val;
}

void IIRFilter::setGain(float val)
{
	pf_gain = val;
}

//lynched from SoX >w>
void IIRFilter::make_poly_from_roots(
	double const * roots, size_t num_roots, float * poly)
{
	size_t i, j;
	poly[0] = 1;
	poly[1] = -roots[0];
	memset(poly + 2, 0, (num_roots + 1 - 2) * sizeof(*poly));
	for (i = 1; i < num_roots; ++i)
		for (j = num_roots; j > 0; --j)
			poly[j] -= poly[j - 1] * roots[i];
}

void IIRFilter::init(int samplerate, int filter_type)
{
	xn1=0;
	xn2=0;
	yn1=0;
	yn2=0;

	//limit to nyquist
	if (pf_freq > ((int)samplerate / 2.0)) pf_freq = (int)(samplerate / 2.0);

	if (filter_type == RIAA_CD)
	{
		//thanks to lvqcl for coeffs
		switch (samplerate)
		{
		case 44100:
			pf_gain = -9.465;
			pf_qfact = 0.4850;
			pf_freq = 5277;
			break;
		case 48000:
			pf_gain = -9.605;
			pf_qfact = 0.4858;
			pf_freq = 5356;
			break;
		case 88200:
			pf_gain = -10.155;
			pf_qfact = 0.4970;
			pf_freq = 5635;
			break;
		case 96000:
			pf_gain = -10.200;
			pf_qfact = 0.4975;
			pf_freq = 5660;
			break;
		case 176400:
			pf_gain = -10.3745;
			pf_qfact = 0.4993;
			pf_freq = 5764;
			break;
		case 192000:
			pf_gain = -10.388;
			pf_qfact = 0.4995;
			pf_freq = 5772;
			break;
		default: // we cannot be here but ...
			pf_gain = -9.465;
			pf_qfact = 0.4850;
			pf_freq = 5277;
			break;
		}
	}
	
	omega = 2 * M_PI * pf_freq/samplerate;
	cs = cos(omega);
	sn = sin(omega);
	A = exp(log(10.0) * pf_gain  / 40);
	beta = 2* sqrt(A);

	a1pha = sn / (2.0 * pf_qfact);

	//Set up filter coefficients according to type
	switch (filter_type)
	{
	case LPF:
		b0 =  (1.0 - cs) / 2.0 ;
		b1 =   1.0 - cs ;
		b2 =  (1.0 - cs) / 2.0 ;
		a0 =   1.0 + a1pha ;
		a1 =  -2.0 * cs ;
		a2 =   1.0 - a1pha ;
		break;
	case HPF:
		b0 =  (1.0 + cs) / 2.0 ;
		b1 = -(1.0 + cs) ;
		b2 =  (1.0 + cs) / 2.0 ;
		a0 =   1.0 + a1pha ;
		a1 =  -2.0 * cs ;
		a2 =   1.0 - a1pha ;
		break;
	case APF:
		b0=1.0-a1pha;
		b1=-2.0*cs;
		b2=1.0+a1pha;
		a0=1.0+a1pha;
		a1=-2.0*cs;
		a2=1.0-a1pha;
		break;
	case BPZPGF:
		b0 =   a1pha ;
		b1 =   0.0 ;
		b2 =  -a1pha ;
		a0 =   1.0 + a1pha ;
		a1 =  -2.0 * cs ;
		a2 =   1.0 - a1pha ;
		break;
	case BPCSGF:
		b0=sn/2.0;
		b1=0.0;
		b2=-sn/2;
		a0=1.0+a1pha;
		a1=-2.0*cs;
		a2=1.0-a1pha;
	break;
	case NOTCH: 
		b0 = 1;
		b1 = -2 * cs;
		b2 = 1;
		a0 = 1 + a1pha;
		a1 = -2 * cs;
		a2 = 1 - a1pha;
		break;
	case RIAA_phono: /* http://www.dsprelated.com/showmessage/73300/3.php */
		if (samplerate == 44100) {
			static const double zeros[] = {-0.2014898, 0.9233820};
			static const double poles[] = {0.7083149, 0.9924091};
			make_poly_from_roots(zeros, (size_t)2, &b0);
			make_poly_from_roots(poles, (size_t)2, &a0);
		}
		else if (samplerate == 48000) {
			static const double zeros[] = {-0.1766069, 0.9321590};
			static const double poles[] = {0.7396325, 0.9931330};
			make_poly_from_roots(zeros, (size_t)2, &b0);
			make_poly_from_roots(poles, (size_t)2, &a0);
		}
		else if (samplerate == 88200) {
			static const double zeros[] = {-0.1168735, 0.9648312};
			static const double poles[] = {0.8590646, 0.9964002};
			make_poly_from_roots(zeros, (size_t)2, &b0);
			make_poly_from_roots(poles, (size_t)2, &a0);
		}
		else if (samplerate == 96000) {
			static const double zeros[] = {-0.1141486, 0.9676817};
			static const double poles[] = {0.8699137, 0.9966946};
			make_poly_from_roots(zeros, (size_t)2, &b0);
			make_poly_from_roots(poles, (size_t)2, &a0);
		}
		{ /* Normalise to 0dB at 1kHz (Thanks to Glenn Davis) */
			double y = 2 * M_PI * 1000 / samplerate ;
			double b_re = b0 + b1 * cos(-y) +b2 * cos(-2 * y);
			double a_re = a0 + a1 * cos(-y) + a2 * cos(-2 * y);
			double b_im = b1 * sin(-y) + b2 * sin(-2 * y);
			double a_im = a1 * sin(-y) + a2 * sin(-2 * y);
			double g = 1 / sqrt((sqr(b_re) + sqr(b_im)) / (sqr(a_re) + sqr(a_im)));
			b0 *= g; b1 *= g; b2 *= g;
		}
		break;
	case PEQ: 
		b0 =   1 + a1pha * A ;
		b1 =  -2 * cs ;
		b2 =   1 - a1pha * A ;
		a0 =   1 + a1pha / A ;
		a1 =  -2 * cs ;
		a2 =   1 - a1pha / A ;
		break; 
	case BBOOST:       
		beta = sqrt((A * A + 1) / 1.0 - (pow((A - 1), 2)));
		b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
		b1 = 2 * A * ((A - 1) - (A + 1) * cs);
		b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
		a0 = ((A + 1) + (A - 1) * cs + beta * sn);
		a1 = -2 * ((A - 1) + (A + 1) * cs);
		a2 = (A + 1) + (A - 1) * cs - beta * sn;
		break;
	case LSH:
		b0 = A * ((A + 1) - (A - 1) * cs + beta * sn);
		b1 = 2 * A * ((A - 1) - (A + 1) * cs);
		b2 = A * ((A + 1) - (A - 1) * cs - beta * sn);
		a0 = (A + 1) + (A - 1) * cs + beta * sn;
		a1 = -2 * ((A - 1) + (A + 1) * cs);
		a2 = (A + 1) + (A - 1) * cs - beta * sn;
		break;
	case RIAA_CD:
	case HSH:
		b0 = A * ((A + 1) + (A - 1) * cs + beta * sn);
		b1 = -2 * A * ((A - 1) + (A + 1) * cs);
		b2 = A * ((A + 1) + (A - 1) * cs - beta * sn);
		a0 = (A + 1) - (A - 1) * cs + beta * sn;
		a1 = 2 * ((A - 1) - (A + 1) * cs);
		a2 = (A + 1) - (A - 1) * cs - beta * sn;
		break;
	default:
		break;
	}
}

float IIRFilter::Process(float samp)
{
	float out, in = 0;
	in = samp;
	out = (b0 * in + b1 * xn1 + b2 * xn2 - a1 * yn1 - a2 * yn2) / a0;
	xn2 = xn1;
	xn1 = in;
	yn2 = yn1;
	yn1 = out;
	return out;
}