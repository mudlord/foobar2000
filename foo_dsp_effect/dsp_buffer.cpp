#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "circular_buffer.h"

#define BUFFER_SIZE 1024

class dsp_buffer : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	unsigned buffered;                          //number of buffered sample pairs ("frames" or audio_chunks)
	circular_buffer<audio_sample>sample_buffer; //the circular buffer to read/write from
	pfc::array_t<audio_sample>sample_array;     //Temporary buffer to store results from/to circular buffer :3
public:
	dsp_buffer() {
		m_rate = m_ch = m_ch_mask = buffered = 0;
	}

	// Every DSP type is identified by a GUID.
	static GUID g_get_guid() {
		// {05193353-F1A7-4BE3-8A35-9BF71A4DBD34}
		static const GUID guid = 
		{ 0x5193353, 0xf1a7, 0x4be3, { 0x8a, 0x35, 0x9b, 0xf7, 0x1a, 0x4d, 0xbd, 0x34 } };
		return guid;
	}

	static void g_get_name(pfc::string_base & p_out) {
		p_out = "Circular buffer test";
	}

	virtual void on_endoftrack(abort_callback & p_abort) {
		
	}

	virtual void on_endofplayback(abort_callback & p_abort) {
		
	}


	virtual bool on_chunk(audio_chunk * chunk, abort_callback & p_abort) {
		t_size sample_count = chunk->get_sample_count();
		audio_sample * source_samples = chunk->get_data();

		if (chunk-> get_srate() != m_rate|| chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			//init
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			sample_buffer.set_size(BUFFER_SIZE*m_ch);
			sample_array.set_size(BUFFER_SIZE*m_ch);
		}
		sample_array.grow_size(BUFFER_SIZE*m_ch);
		while (sample_count > 0)
		{    
			int todo = min(BUFFER_SIZE - buffered, sample_count);
			sample_buffer.write(source_samples,todo*m_ch);
			source_samples += todo * m_ch;
			buffered += todo;
			sample_count -= todo;
			if (buffered == BUFFER_SIZE)
			{
				//insert processing here :3
				//Atm we just copy the samples from the circular buffer to the output
				audio_sample * samples = sample_array.get_ptr(); //Get pointer to temporary buffer
				int done = sample_buffer.read(samples,buffered*m_ch); //returns total number of audio samples
				audio_chunk *out_chunk = insert_chunk(done); //Say to FB2K that we want to add our own samples.
				out_chunk->set_data_32(samples,buffered, m_ch, m_rate); //Add our newly read samples from the circular buffer, yay!
				buffered = 0;
			}
		// Return false to keep the input chunk from being added to the output, since we do that ourselves...
		}
		return false;
	}

	virtual void flush() {
		m_rate = m_ch = m_ch_mask = buffered = 0;
	}

	virtual double get_latency() {
		//We need to return the latency of the circular buffer being used.
		//So to get that, we divide the number of samples in the buffer, by the current sample rate.
		return (double)sample_buffer.data_available() / m_rate;
	}

	virtual bool need_track_change_mark() {
		return false;
	}
};
//static dsp_factory_nopreset_t<dsp_buffer> foo_dsp_tutorial_nopreset;
