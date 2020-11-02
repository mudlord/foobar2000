#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include "circular_buffer.h"
#include "samplerate.h"
#define BUFFER_SIZE 1024
static void RunDSPConfigPopup( const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback );
class dsp_resample_src : public dsp_impl_base
{
	int m_rate, m_ch, m_ch_mask;
	int new_rate; 
	int q_setting;
	circular_buffer<audio_sample>sample_buffer;
	pfc::array_t<audio_sample>in_samples;
	pfc::array_t<audio_sample>out_samples;
	unsigned buffered;
	SRC_STATE *resampler; 

public:
	dsp_resample_src(dsp_preset const & in) :m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 ),buffered(0),q_setting(5),new_rate(48000) {
		resampler = NULL;
		parse_preset( q_setting, new_rate ,in);
	}

	~dsp_resample_src(){
			if (resampler) src_delete(resampler);
			resampler = NULL;
	}

	// Every DSP type is identified by a GUID.
	static GUID g_get_guid() {
		// {5F9E48EA-EB39-471B-A6FB-716482905350}
		static const GUID guid = 
		{ 0x5f9e48ea, 0xeb39, 0x471b, { 0xa6, 0xfb, 0x71, 0x64, 0x82, 0x90, 0x53, 0x50 } };

		return guid;
	}

	static void g_get_name(pfc::string_base & p_out) {
		p_out = "Resampler (Secret Rabbit Code)";
	}

	virtual void on_endoftrack(abort_callback & p_abort) {
		flushwrite();
	}

	void flushwrite()
	{
		if (buffered)
		{
			SRC_DATA	src_data;
			audio_sample * input = in_samples.get_ptr();
			audio_sample * output = out_samples.get_ptr();
			int done = sample_buffer.read(input,buffered*m_ch);//number of individual samples
			t_size in_sample_count = buffered;                  //number of sample pairs
			t_size out_sample_count = buffered; //initialize
			while (in_sample_count > 0)
			{
				unsigned todo = min(BUFFER_SIZE, in_sample_count);
				src_data.data_in = input;
				src_data.data_out = output;
				src_data.input_frames = todo;
				src_data.output_frames = out_sample_count;
				src_data.end_of_input = 0;
				src_data.src_ratio = (1.0 * new_rate) / m_rate;
				src_process(resampler,&src_data);
				todo = src_data.input_frames_used;
				out_sample_count = src_data.output_frames_gen;
				audio_chunk *out_chunk = insert_chunk(out_sample_count); 
				out_chunk->set_data(output,out_sample_count, m_ch, new_rate);
				input += todo*m_ch;
				in_sample_count -= todo;
			}
			buffered = 0;
		}
	}

	virtual void on_endofplayback(abort_callback & p_abort) {
		flushwrite();
	}

	virtual bool on_chunk(audio_chunk * chunk, abort_callback & p_abort) {
		t_size sample_count = chunk->get_sample_count();
		audio_sample * source_samples = chunk->get_data();
		SRC_DATA	src_data;
	
		if (chunk-> get_srate() != m_rate|| chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			sample_buffer.set_size(BUFFER_SIZE*m_ch);
			in_samples.set_size(0);
			out_samples.set_size(0);
			int err=0;
			if (resampler) src_delete(resampler);
			resampler = NULL;
			resampler = src_new(q_setting,m_ch,&err);
			in_samples.grow_size(BUFFER_SIZE*m_ch);
			out_samples.grow_size(BUFFER_SIZE*m_ch);
			if (err) return 0;
		}
		
		while (sample_count > 0)
		{    
			int todo = min(BUFFER_SIZE - buffered, sample_count);
			bool canWrite = sample_buffer.write(source_samples,todo*m_ch);
			source_samples += todo * m_ch;
			buffered += todo;
			sample_count -= todo;
			if (buffered == BUFFER_SIZE)
			{
				flushwrite();
			}
		}
		return false;
	}

	virtual void flush() {
		src_reset(resampler);
		m_rate = m_ch = m_ch_mask = buffered = 0;
	}

	virtual double get_latency() {
		return (resampler) ? ((double)buffered / (double)new_rate): 0;
	}

	virtual bool need_track_change_mark() {
		return false;
	}

	static bool g_get_default_preset( dsp_preset & p_out )
	{
		make_preset( 0,48000, p_out );
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
		catch(exception_io_data) {q_setting = 0;new_rate=48000;}
	}
};

const long samplerates[] = {8000, 11025, 16000, 22050, 24000, 32000, 44100, 48000, 64000, 88200, 96000, 176400, 192000};
const TCHAR* samplerates_s[] = {L"8000", L"11025", L"16000", L"22050",L"24000",L"32000",
	L"44100",L"48000", L"64000",L"88200",L"96000", L"176400",L"192000"};
const TCHAR * q_settings []={L"Best",L"Medium", L"Fastest" ,L"Zero Order Hold", L"Linear"};
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
		dsp_resample_src::parse_preset(q_setting,new_rate, m_initData);
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
		dsp_resample_src::make_preset( q_level,samplerates[new_srate], preset );
		m_callback.on_preset_changed( preset );
	}

	void OnSampleRateChange( UINT, int id, CWindow )
	{
		int new_srate = samplerate_select.GetCurSel();
		int q_level = quality_select.GetCurSel();
		dsp_preset_impl preset;
		dsp_resample_src::make_preset( q_level,samplerates[new_srate], preset );
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

static dsp_factory_t<dsp_resample_src> foo_dsp_srcresample;

