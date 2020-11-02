#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "iirfilters.h"


DECLARE_COMPONENT_VERSION("De-emphasis postprocessor and DSP set", "0.1", "Copyright(C) 2017 mudlord\n"
"Usage: create tag PRE_EMPHASIS or PRE-EMPHASIS with value \"1\", \"on\" or \"yes\"\n"
);

// This will prevent users from renaming the component around (important for proper troubleshooter behaviors) or loading multiple instances of it.
VALIDATE_COMPONENT_FILENAME("foo_dsp_deemph.dll");

class dsp_iir_cddemph : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	int p_freq; //40.0, 13000.0 (Frequency: Hz)
	int p_gain; //gain
	int p_type; //filter type
	bool iir_enabled;
	pfc::array_t<IIRFilter> m_buffers;
public:
	static GUID g_get_guid()
	{
		// {FEA092A6-EA54-4f62-B180-4C88B9EB2B67}
		// {943393FF-A077-4D76-9832-51CA19A75FDA}
		static const GUID guid =
		{ 0x943393ff, 0xa077, 0x4d76,{ 0x98, 0x32, 0x51, 0xca, 0x19, 0xa7, 0x5f, 0xda } };

		return guid;
	}

	dsp_iir_cddemph() :m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ), p_freq(400), p_gain(10), p_type(0)
	{
		iir_enabled = true;
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "RIAA CD deemphasis filter"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			m_buffers.set_count( 0 );
			m_buffers.set_count( m_ch );
			for ( unsigned i = 0; i < m_ch; i++ )
			{
				IIRFilter & e = m_buffers[ i ];
				e.setFrequency(p_freq);
				e.setQuality(0.707);
				e.setGain(p_gain);
				e.init(m_rate,RIAA_CD);
			}
		}

		for ( unsigned i = 0; i < m_ch; i++ )
		{
			IIRFilter & e = m_buffers[ i ];
			audio_sample * data = chunk->get_data() + i;
			for ( unsigned j = 0, k = chunk->get_sample_count(); j < k; j++ )
			{
				*data = e.Process(*data);
				data += m_ch;
			}
		}

		return true;
	}

	void on_endofplayback( abort_callback & ) { }
	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		m_buffers.set_count( 0 );
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
	}
	double get_latency()
	{
		return 0;
	}
	bool need_track_change_mark()
	{
		return false;
	}
};
class dsp_iir_phonodemph : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	int p_freq; //40.0, 13000.0 (Frequency: Hz)
	int p_gain; //gain
	int p_type; //filter type
	bool iir_enabled;
	pfc::array_t<IIRFilter> m_buffers;
public:
	static GUID g_get_guid()
	{
		static const GUID guid =
		{ 0x8ded65cf, 0x196b, 0x40ae,{ 0x94, 0x76, 0x3f, 0x5d, 0xe9, 0xf8, 0x34, 0x4e } };


		return guid;
	}

	dsp_iir_phonodemph() :m_rate(0), m_ch(0), m_ch_mask(0), p_freq(400), p_gain(10), p_type(0)
	{
		iir_enabled = true;
	}

	static void g_get_name(pfc::string_base & p_out) { p_out = "RIAA phono deemphasis filter"; }

	bool on_chunk(audio_chunk * chunk, abort_callback &)
	{
		if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			m_buffers.set_count(0);
			m_buffers.set_count(m_ch);
			for (unsigned i = 0; i < m_ch; i++)
			{
				IIRFilter & e = m_buffers[i];
				e.setFrequency(p_freq);
				e.setQuality(0.707);
				e.setGain(p_gain);
				e.init(m_rate, RIAA_phono);
			}
		}

		for (unsigned i = 0; i < m_ch; i++)
		{
			IIRFilter & e = m_buffers[i];
			audio_sample * data = chunk->get_data() + i;
			for (unsigned j = 0, k = chunk->get_sample_count(); j < k; j++)
			{
				*data = e.Process(*data);
				data += m_ch;
			}
		}

		return true;
	}

	void on_endofplayback(abort_callback &) { }
	void on_endoftrack(abort_callback &) { }

	void flush()
	{
		m_buffers.set_count(0);
		m_rate = 0;
		m_ch = 0;
		m_ch_mask = 0;
	}
	double get_latency()
	{
		return 0;
	}
	bool need_track_change_mark()
	{
		return false;
	}
};

class deemph_postprocessor_instance1 : public decode_postprocessor_instance
{
pfc::array_t<IIRFilter> filter;
unsigned srate, nch, channel_config;
bool enabled;
private:
	void init()
	{
		filter.set_size(nch);
		for (unsigned h = 0; h < nch; h++)
		{
			//these do not matter, only samplerate does
			filter[h].setFrequency(400);
			filter[h].setQuality(0);
			filter[h].setGain(0);
			filter[h].init(srate,RIAA_CD);
		}
	}

	void cleanup()
	{
		filter.set_size(0);
		srate = 0;
		nch = 0;
		channel_config = 0;
	}
public:
	deemph_postprocessor_instance1()
	{
		cleanup();
	}

	virtual bool run(dsp_chunk_list & p_chunk_list, t_uint32 p_flags, abort_callback & p_abort)
	{

		for (unsigned i = 0; i < p_chunk_list.get_count(); )
		{
			audio_chunk * chunk = p_chunk_list.get_item(i);

			if (srate != chunk->get_sample_rate() || nch != chunk->get_channels() || channel_config != chunk->get_channel_config())
			{
				srate = chunk->get_sample_rate();
				nch = chunk->get_channels();
				channel_config = chunk->get_channel_config();
				init();
			}

			if (srate != 44100) return false;

			for (unsigned k = 0; k < nch; k++)
			{
				audio_sample * data = chunk->get_data() + k;
				for (unsigned j = 0, l = chunk->get_sample_count(); j < l; j++)
				{
					*data = filter[k].Process(*data);
					data += nch;
				}
			}
			i++;
		}
		return true;
	}

	virtual bool get_dynamic_info(file_info & p_out)
	{
		return false;
	}

	virtual void flush()
	{
		cleanup();
	}

	virtual double get_buffer_ahead()
	{
		return 0;
	}
};


class deemph_postprocessor_entry : public decode_postprocessor_entry
{
public:
	virtual bool instantiate(const file_info & info, decode_postprocessor_instance::ptr & out)
	{
		int sr = (int)info.info_get_int("samplerate");
		if (sr != 44100 && sr != 48000 && sr != 88200 && sr != 96000 && sr != 176400 && sr != 192000) return false;


		const char* enabled = info.meta_get("pre_emphasis", 0);
		if (enabled == NULL) enabled = info.meta_get("pre-emphasis", 0);
		if (enabled == NULL) return false;

		if (pfc::stricmp_ascii(enabled, "1") == 0 || pfc::stricmp_ascii(enabled, "on") == 0 || pfc::stricmp_ascii(enabled, "yes") == 0)
		{
			console::print("Pre-emphasis detected and enabled in track. Running filter");
			out = new service_impl_t < deemph_postprocessor_instance1 >;
			return true;
		}
		return false;
	}
};
static service_factory_single_t<deemph_postprocessor_entry> g_deemph_postprocessor_entry_factory;

static dsp_factory_nopreset_t<dsp_iir_cddemph> foo_dsp_tutorial_nopreset;
static dsp_factory_nopreset_t<dsp_iir_phonodemph> foo_dsp_tutorial_nopreset2;