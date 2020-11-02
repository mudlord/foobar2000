#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "speex_resampler.h" 
#define BUFFER_SIZE 1024
static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );
class dsp_resample_speex : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	int new_rate; 
	int q_setting;
	t_size in_samples_total, out_samples_total;
	pfc::array_t<audio_sample>out_samples;
	SpeexResamplerState *resampler; 
public:
	dsp_resample_speex(dsp_preset const & in){
		m_rate = m_ch = m_ch_mask =in_samples_total = out_samples_total = 0;
		new_rate = 48000;

		resampler = NULL;
		parse_preset( q_setting, new_rate ,in);
	}

	~dsp_resample_speex(){
			if (resampler) speex_resampler_destroy(resampler);
			resampler = NULL;
	}

	// Every DSP type is identified by a GUID.
	static GUID g_get_guid() {
		// {C2D19FE8-5AFB-4C91-B39D-C54643F35B81}
		static const GUID guid = 
		{ 0xc2d19fe8, 0x5afb, 0x4c91, { 0xb3, 0x9d, 0xc5, 0x46, 0x43, 0xf3, 0x5b, 0x81 } };
		return guid;
	}

	static void g_get_name(pfc::string_base & p_out) {
		p_out = "Resampler (Speex)";
	}

	void flushwrite()
	{
		size_t in_samples_used, out_samples_gen, sample_count;
		int spxs_error;
		if (!resampler) return;
		sample_count = speex_resampler_get_input_latency(resampler);
		audio_sample * output = out_samples.get_ptr();

		while(1)
		{
			in_samples_used = sample_count; out_samples_gen = BUFFER_SIZE;
			spxs_error = speex_resampler_process_interleaved_float(resampler, NULL, &in_samples_used,output, &out_samples_gen);
			if (spxs_error) { console::error(speex_resampler_strerror(spxs_error)); break; }
			sample_count -= in_samples_used;

			if (out_samples_gen != 0)
			{
				audio_chunk * out = insert_chunk(out_samples_gen*m_ch);
				out->set_data(output, out_samples_gen, m_ch, new_rate, m_ch_mask);
			}
			if (sample_count == 0 && out_samples_gen == 0) break;
		}
		speex_resampler_reset_mem(resampler); speex_resampler_skip_zeros(resampler);
		in_samples_total = out_samples_total = 0;
		return;
	}

	virtual void on_endoftrack(abort_callback & p_abort) {
		 flushwrite();
	}

	virtual void on_endofplayback(abort_callback & p_abort) {
		 flushwrite();
	}

	virtual bool on_chunk(audio_chunk * chunk, abort_callback & p_abort) {
		t_size sample_count = chunk->get_sample_count();
		audio_sample * source_samples = chunk->get_data();
		spx_uint32_t samples_used, samples_gen;

	
		if (chunk-> get_srate() != m_rate|| chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			out_samples.set_size(0);
			int err=0;
			if (resampler) speex_resampler_destroy(resampler);
			resampler = NULL;
			resampler = speex_resampler_init(m_ch, m_rate, new_rate, q_setting, &err); 
			if (err) return 0;
		}
	
		out_samples.grow_size(BUFFER_SIZE*m_ch);
		audio_sample * output = out_samples.get_ptr();

		in_samples_total += sample_count;

		while (sample_count > 0)
		{   
			samples_used = sample_count; samples_gen = BUFFER_SIZE;
			int res = speex_resampler_process_interleaved_float(resampler, source_samples, &samples_used, output, &samples_gen);
			if (res) { console::error(speex_resampler_strerror(res)); break; }
			sample_count -= samples_used;
			source_samples += samples_used * m_ch;

			out_samples_total += samples_gen;
			if (samples_gen != 0)
			{
				//send ANY samples we get to out, don't buffer it seperately.
				audio_chunk * out = insert_chunk(samples_gen*m_ch);
				out->set_data(output, samples_gen, m_ch, new_rate, m_ch_mask);
			}
		}

		while (in_samples_total > m_rate && out_samples_total > new_rate)
		{
			in_samples_total -= m_rate;
			out_samples_total -= new_rate;
		}

		return false;
	}

	virtual void flush() {
		if (!resampler) return;
		speex_resampler_reset_mem(resampler); 
		speex_resampler_skip_zeros(resampler);
		m_rate = m_ch = m_ch_mask =in_samples_total = out_samples_total = 0;
	}

	virtual double get_latency() {
		if (m_rate && new_rate)
		return double(in_samples_total)/double(m_rate) - double(out_samples_total)/double(new_rate);
		else return 0;
	}

	virtual bool need_track_change_mark() {
		return false;
	}

	static bool g_get_default_preset( dsp_preset & p_out )
	{
		make_preset( 5,48000, p_out );
		return true;
	}
	static void g_show_config_popup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
	{
		::RunDSPConfigPopup( p_data, p_parent, p_callback );
	}
	static bool g_have_config_popup() { return true; }
	static void make_preset( int q_setting, int new_rate, dsp_preset & out )
	{
		dsp_preset_builder builder; 
		builder << q_setting;
		builder << new_rate;
		builder.finish( g_get_guid(), out );
	}                        
	static void parse_preset(int & q_setting, int & new_rate, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			parser >> q_setting;
			parser >> new_rate;
		}
		catch(exception_io_data) {q_setting = 5;new_rate=48000;}
	}
};



class dsp_resample_speex_entry : public resampler_entry
{
public:
	dsp_resample_speex_entry() {

	};
};


const long samplerates[] = {8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, 176400, 192000};
const TCHAR* samplerates_s[] = {L"8000", L"11025", L"16000", L"22050",L"24000",L"32000",
	L"44100",L"48000", L"64000",L"88200",L"96000", L"176400",L"192000"};
const TCHAR * q_settings []={L"0",L"1", L"2" ,L"3 (VoIP)", L"4 (Default)", L"5 (Desktop)", L"6", L"7", L"8", L"9", L"10"};
class CMyDSPPopupResample : public CDialogImpl<CMyDSPPopupResample>
{
public:
	CMyDSPPopupResample( const dsp_preset & initData, dsp_preset_edit_callback & callback ) : m_initData( initData ), m_callback( callback ) { }
	enum { IDD = IDD_RESAMPLER };
	BEGIN_MSG_MAP( CMyDSPPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDOK, BN_CLICKED, OnButton )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnButton )
		COMMAND_HANDLER_EX(IDC_SAMPLERATE, CBN_SELCHANGE, OnSampleRateChange)
		COMMAND_HANDLER_EX(IDC_RESAMPLEQ, CBN_SELCHANGE, OnQChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{
		int new_rate; 
		int q_setting;
		TCHAR temp[16] = {0};
		samplerate_select = GetDlgItem(IDC_SAMPLERATE);
		quality_select = GetDlgItem(IDC_RESAMPLEQ);
		int n;
		dsp_resample_speex::parse_preset(q_setting,new_rate, m_initData);
		for (n=0;n<tabsize(samplerates_s);n++)
		{
			samplerate_select.AddString(samplerates_s[n]);
		}
		_itow(new_rate,temp,10);
		int pos = samplerate_select.FindString(0,temp);
		samplerate_select.SetCurSel(pos);
		for (n=0;n<tabsize(q_settings);n++)
		{
			quality_select.AddString(q_settings[n]);
		}
		quality_select.SetCurSel(q_setting);
		return TRUE;
	}

	void OnQChange( UINT, int id, CWindow )
	{
		int new_srate = samplerate_select.GetCurSel();
		int q_level = quality_select.GetCurSel();
		dsp_preset_impl preset;
		dsp_resample_speex::make_preset( q_level,samplerates[new_srate], preset );
		m_callback.on_preset_changed( preset );
	}

	void OnSampleRateChange( UINT, int id, CWindow )
	{
		int new_srate = samplerate_select.GetCurSel();
		int q_level = quality_select.GetCurSel();
		dsp_preset_impl preset;
		dsp_resample_speex::make_preset( q_level,samplerates[new_srate], preset );
		m_callback.on_preset_changed( preset );
	}

	void OnButton( UINT, int id, CWindow )
	{
		EndDialog( id );
	}
	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;
	CComboBox samplerate_select, quality_select;
};
static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback )
{
	CMyDSPPopupResample popup( p_data, p_callback );
	if ( popup.DoModal(p_parent) != IDOK ) p_callback.on_preset_changed( p_data );
}


//static service_factory_t<dsp_resample_speex_entry>	foo_dsp_speex;
static dsp_factory_t<dsp_resample_speex> foo_dsp_speexresample;

