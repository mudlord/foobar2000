#include "../helpers/foobar2000+atl.h"
#include "../../libPPUI/win32_utility.h"
#include "../../libPPUI/win32_op.h" // WIN32_OP()
#include "../helpers/atl-misc.h"// ui_element_impl
#include "../helpers/BumpableElem.h"
#include "dmodsp.h"
#include "resource.h"

static void RunConfigPopupChorus(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);

static const GUID guid_dmo_chorus =
{ 0xb904dac3, 0x5dae, 0x4e9b,{ 0x96, 0xb5, 0x3d, 0x92, 0xbe, 0x33, 0xbf, 0x38 } };
// {4663F983-BD2A-4C3D-B651-1C17786FB9F6}
static const GUID guid_dmo_compressor =
{ 0x4663f983, 0xbd2a, 0x4c3d, { 0xb6, 0x51, 0x1c, 0x17, 0x78, 0x6f, 0xb9, 0xf6 } };
// {5ED5FE63-8F05-4F6C-A0CC-630B238012B7}
static const GUID guid_dmo_distort =
{ 0x5ed5fe63, 0x8f05, 0x4f6c, { 0xa0, 0xcc, 0x63, 0xb, 0x23, 0x80, 0x12, 0xb7 } };
// {0B0D6BA5-0C56-4501-9238-FBA0E8ED4F89}
static const GUID guid_dmo_gargle =
{ 0xb0d6ba5, 0xc56, 0x4501, { 0x92, 0x38, 0xfb, 0xa0, 0xe8, 0xed, 0x4f, 0x89 } };
// {5E0BC3FF-F9C1-4FA1-9625-F7D74C92D39C}
static const GUID guid_dmo_waves =
{ 0x5e0bc3ff, 0xf9c1, 0x4fa1, { 0x96, 0x25, 0xf7, 0xd7, 0x4c, 0x92, 0xd3, 0x9c } };




class dsp_dmo_chorus : public dsp_impl_base
{
	enum Parameters
	{
		kChorusWetDryMix = 0,
		kChorusDepth,
		kChorusFrequency,
		kChorusWaveShape,
		kChorusPhase,
		kChorusFeedback,
		kChorusDelay,
		kChorusNumParameters
	};
	float m_param[kChorusNumParameters];
	int m_rate, m_ch, m_ch_mask;
	bool enabled;
	DMOFilter dmo_filt;
public:
	static GUID g_get_guid()
	{
		// {B904DAC3-5DAE-4E9B-96B5-3D92BE33BF38}
		return guid_dmo_chorus;
	}

	dsp_dmo_chorus(dsp_preset const & in) :m_rate( 0 ), m_ch( 0 ), m_ch_mask( 0 )
	{
		parse_preset(m_param, enabled, in);
		m_param[kChorusFeedback] = (m_param[kChorusFeedback] + 99.0f) / 198.0f;
		enabled = true;
	}
	~dsp_dmo_chorus()
	{
		dmo_filt.deinit();
	}

	static void g_get_name( pfc::string_base & p_out ) { p_out = "Chorus (DirectX)"; }

	bool on_chunk( audio_chunk * chunk, abort_callback & )
	{
		if ( chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask )
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			dmo_filt.deinit();
			if (m_ch <=  2)
			{
				dmo_filt.init(m_rate, DMO_Type::CHORUS, m_ch);
				for (int i = 0; i < kChorusNumParameters; i++)
				dmo_filt.setparameter(i, m_param[i]);
			}
			
		}
		if (m_ch <= 2)
		{
			audio_sample * data = chunk->get_data();
			unsigned count = chunk->get_data_size();
			dmo_filt.Process(data, count);
		}
	

		return true;
	}

	void on_endofplayback( abort_callback & ) { }
	void on_endoftrack( abort_callback & ) { }

	void flush()
	{
		dmo_filt.deinit();
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

	static bool g_get_default_preset(dsp_preset & p_out)
	{
		float params[kChorusNumParameters] =
		{
		0.5f,
		0.2f,
		0.11f,
		1.0f,
		0.75f,
		(40.0f + 99.0f) / 198.0f,
		0.8f
		};
		

		make_preset(params, true, p_out);
		return true;
	}
	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		::RunConfigPopupChorus(p_data, p_parent, p_callback);
	}
	static bool g_have_config_popup() { return true; }

	static void make_preset(float params[kChorusNumParameters],bool enabled, dsp_preset & out)
	{
		dsp_preset_builder builder;
		for (int i = 0; i < kChorusNumParameters; i++)
		builder << params[i];
		builder << enabled;
		builder.finish(g_get_guid(), out);
	}

	static void parse_preset(float *params, bool & enabled, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			for (int i = 0; i < kChorusNumParameters; i++)
			{
				parser >> params[i];
			}
			parser >> enabled;
		}
		catch (exception_io_data) {
			params[kChorusWetDryMix] = 0.5f;
		    params[kChorusDepth] = 0.1f;
			params[kChorusFrequency] = 0.11f;
			params[kChorusWaveShape] = 1.0f;
			params[kChorusPhase] = 0.75f;
			params[kChorusFeedback] = (40.0f + 99.0f) / 198.0f;
			params[kChorusDelay] = 0.8f;
			enabled = true;
		}
	}
};

class CMyDSPPopupChorus : public CDialogImpl<CMyDSPPopupChorus>
{
public:
	CMyDSPPopupChorus(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }
	enum { IDD = IDD_DMOCHORUS };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kChorusWetDryMix = 0,
		kChorusDepth,
		kChorusFrequency,
		kChorusWaveShape,
		kChorusPhase,
		kChorusFeedback,
		kChorusDelay,
		kChorusNumParameters
	};

	BEGIN_MSG_MAP(CMyDSPPopupChorus)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_DMOCHORUSPHASE, CBN_SELCHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_DMOCHORUSWAVETYPE, CBN_SELCHANGE, OnChange)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{

		CWindow w = GetDlgItem(IDC_DMOCHORUSWAVETYPE);
		CWindow w2 = GetDlgItem(IDC_DMOCHORUSPHASE);
		uSendMessageText(w, CB_ADDSTRING, 0, "Triangle");
		uSendMessageText(w, CB_ADDSTRING, 0, "Sine");
		
		uSendMessageText(w2, CB_ADDSTRING, 0, "-180");
		uSendMessageText(w2, CB_ADDSTRING, 0, "-90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "0");
		uSendMessageText(w2, CB_ADDSTRING, 0, "90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "180");


		slider_delayms = GetDlgItem(IDC_DMOCHORUSDELAYMS);
		slider_delayms.SetRange(FreqMin, FreqMax);
		slider_depthms = GetDlgItem(IDC_DMOCHORUSDEPTHMS);
		slider_depthms.SetRange(10, 100);
		slider_freq = GetDlgItem(IDC_DMOCHORUSLFOFREQ);
		slider_freq.SetRange(0, 1000);
		slider_drywet = GetDlgItem(IDC_DMOCHORUSDRYWET);
		slider_drywet.SetRange(0, 100);
		slider_feedback = GetDlgItem(IDC_DMOCHORUSFEEDBACK);
		slider_feedback.SetRange(0, 100);
		{
			bool enabled;
			dsp_dmo_chorus::parse_preset(params, enabled, m_initData);
			slider_delayms.SetPos((double)(100 * params[kChorusDelay]));
			slider_depthms.SetPos((double)(100 * params[kChorusDepth]));
			slider_freq.SetPos((double)(100 * params[kChorusFrequency]));
			slider_drywet.SetPos((double)(100 * params[kChorusWetDryMix]));
			slider_feedback.SetPos((double)(100 * params[kChorusFeedback]));
			RefreshLabel(params[kChorusDelay], params[kChorusDepth], 
			params[kChorusFrequency], params[kChorusWetDryMix],params[kChorusFeedback]);
		}
		int shape = (int)params[kChorusWaveShape];
		::SendMessage(w, CB_SETCURSEL, shape, 0);
		int phase = ((int)round(params[kChorusPhase]));
		::SendMessage(w2, CB_SETCURSEL,phase , 0);

		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnChange(UINT, int id, CWindow)
	{
		params[kChorusDelay] = slider_delayms.GetPos() / 100.0;
		params[kChorusDepth] = slider_depthms.GetPos() / 100.0;
		params[kChorusFrequency] = slider_freq.GetPos() / 100.0;
		params[kChorusWetDryMix] = slider_drywet.GetPos() / 100.0;
		params[kChorusFeedback] = slider_feedback.GetPos() / 100.0;

		params[kChorusWaveShape] = SendDlgItemMessage(IDC_DMOCHORUSWAVETYPE, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_DMOCHORUSPHASE, CB_GETCURSEL);
		params[kChorusPhase] = round(val * 4.0f) / 4.0f;


		dsp_preset_impl preset;
		dsp_dmo_chorus::make_preset(params, true, preset);
		m_callback.on_preset_changed(preset);
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);

	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		params[kChorusDelay] = slider_delayms.GetPos() / 100.0;
	    params[kChorusDepth] = slider_depthms.GetPos() / 100.0;
		params[kChorusFrequency] = slider_freq.GetPos() / 100.0;
    	params[kChorusWetDryMix] = slider_drywet.GetPos() / 100.0;
		params[kChorusFeedback] = slider_feedback.GetPos() / 100.0;

		params[kChorusWaveShape]  = SendDlgItemMessage(IDC_DMOCHORUSWAVETYPE, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_DMOCHORUSPHASE, CB_GETCURSEL);
		params[kChorusPhase] = round(val * 4.0f) / 4.0f;

		if (LOWORD(nSBCode) != SB_THUMBTRACK)
		{
			dsp_preset_impl preset;
			dsp_dmo_chorus::make_preset(params, true, preset);
			m_callback.on_preset_changed(preset);
		}
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);

	}

	void RefreshLabel(float delay_ms, float depth_ms, float lfo_freq, float drywet,float feedback)
	{
		pfc::string_formatter msg;
		msg << "Delay: ";
		msg << pfc::format_float(delay_ms, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDELAYLAB, msg);
		msg.reset();
		msg << "Depth: ";
		msg << pfc::format_int(100 * depth_ms) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDEPTHMSLAB, msg);
		msg.reset();
		msg << "LFO Frequency: ";
		msg << pfc::format_float(lfo_freq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DMOCHORUSLFOFREQLAB, msg);
		msg.reset();
		msg << "Wet/Dry Mix : ";
		msg << pfc::format_int(100 * drywet) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDRYWETLAB, msg);
		msg.reset();
		msg << "Feedback : ";
		msg << pfc::format_int(100 * feedback) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSFEEDBACKLAB, msg);

	}

	const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback & m_callback;
	float params[kChorusNumParameters];
	CTrackBarCtrl slider_drywet,slider_depthms, slider_freq,slider_feedback,slider_delayms ;
	CButton m_buttonEchoEnabled;
};


// {31438FD7-07D0-43DE-A588-90B67D5D5AF1}
static const GUID guid_choruselem =
{ 0x31438fd7, 0x7d0, 0x43de, { 0xa5, 0x88, 0x90, 0xb6, 0x7d, 0x5d, 0x5a, 0xf1 } };


class uielem_chorus : public CDialogImpl<uielem_chorus>, public ui_element_instance {
public:
	uielem_chorus(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
		echo_enabled = true;
	}
	enum { IDD = IDD_DMOCHORUS1 };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kChorusWetDryMix = 0,
		kChorusDepth,
		kChorusFrequency,
		kChorusWaveShape,
		kChorusPhase,
		kChorusFeedback,
		kChorusDelay,
		kChorusNumParameters
	};
private:

	BEGIN_MSG_MAP_EX(uielem_chorus)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_CHORUSENABLED, BN_CLICKED, OnEnabledToggle)
		MSG_WM_HSCROLL(OnScroll)
		COMMAND_HANDLER_EX(IDC_DMOCHORUSWAVETYPE2, CBN_SELCHANGE, OnChange)
		COMMAND_HANDLER_EX(IDC_DMOCHORUSPHASE2, CBN_SELCHANGE, OnChange)
		END_MSG_MAP()

	

	void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
	HWND get_wnd() { return m_hWnd; }
	void set_configuration(ui_element_config::ptr config) {
		shit = parseConfig(config);
		if (m_hWnd != NULL) {
			ApplySettings();
		}
		m_callback->on_min_max_info_change();
	}
	ui_element_config::ptr get_configuration() { return makeConfig(); }
	static GUID g_get_guid() {
		return guid_choruselem;
	}
	static void g_get_name(pfc::string_base & out) { out = "Chorus (DirectX)"; }
	static ui_element_config::ptr g_get_default_configuration() {
		return makeConfig(true);
	}
	static const char* g_get_description() { return "Modifies the 'Chorus' DSP effect."; }
	static GUID g_get_subclass() {
		return ui_element_subclass_dsp;
	}

	ui_element_min_max_info get_min_max_info() {
		ui_element_min_max_info ret;

		// Note that we play nicely with separate horizontal & vertical DPI.
		// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
		CSize DPI = QueryScreenDPIEx(*this);

		if (DPI.cx <= 0 || DPI.cy <= 0) { // sanity
			DPI = CSize(96, 96);
		}


		ret.m_min_width = MulDiv(440, DPI.cx, 96);
		ret.m_min_height = MulDiv(220, DPI.cy, 96);
		ret.m_max_width = MulDiv(440, DPI.cx, 96);
		ret.m_max_height = MulDiv(220, DPI.cy, 96);

		// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
		ret.adjustForWindow(*this);

		return ret;
	}

private:
	void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
	bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

	void EchoDisable() {
		static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_chorus);
	}

	void EchoEnable(float params[], bool echo_enabled) {
		dsp_preset_impl preset;
		dsp_dmo_chorus::make_preset(params, echo_enabled, preset);
		static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
	}

	void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		if (IsEchoEnabled()) {
			GetConfig();
			dsp_preset_impl preset;
			dsp_dmo_chorus::make_preset(params, echo_enabled, preset);
			//yes change api;
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}
		else {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_chorus);
		}

	}

	void OnScroll(UINT scrollID, int pos, CWindow window)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			if (LOWORD(scrollID) != SB_THUMBTRACK)
			{
				EchoEnable(params, echo_enabled);
			}
		}

	}

	void OnChange(UINT, int id, CWindow)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			OnConfigChanged();
		}
	}

	void DSPConfigChange(dsp_chain_config const& cfg)
	{
		if (!m_ownEchoUpdate && m_hWnd != NULL) {
			ApplySettings();
		}
	}

	//set settings if from another control
	void ApplySettings()
	{
		dsp_preset_impl preset;
		if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_dmo_chorus, preset)) {
			SetEchoEnabled(true);
			dsp_dmo_chorus::parse_preset(params, echo_enabled, preset);
			SetEchoEnabled(echo_enabled);
			SetConfig();
		}
		else {
			SetEchoEnabled(false);
			SetConfig();
		}
	}

	void OnConfigChanged() {
		if (IsEchoEnabled()) {
			EchoEnable(params, echo_enabled);
		}
		else {
			EchoDisable();
		}

	}


	void GetConfig()
	{
		params[kChorusDelay] = slider_delayms.GetPos() / 100.0;
		params[kChorusDepth] = slider_depthms.GetPos() / 100.0;
		params[kChorusFrequency] = slider_freq.GetPos() / 100.0;
		params[kChorusWetDryMix] = slider_drywet.GetPos() / 100.0;
		params[kChorusFeedback] = slider_feedback.GetPos() / 100.0;

		params[kChorusWaveShape] = SendDlgItemMessage(IDC_DMOCHORUSWAVETYPE2, CB_GETCURSEL);
		int val = SendDlgItemMessage(IDC_DMOCHORUSPHASE2, CB_GETCURSEL);
		params[kChorusPhase] = round(val * 4.0f) / 4.0f;
		echo_enabled = IsEchoEnabled();
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);
	}

	void SetConfig()
	{
		CWindow w = GetDlgItem(IDC_DMOCHORUSWAVETYPE2);
		CWindow w2 = GetDlgItem(IDC_DMOCHORUSPHASE2);
		slider_delayms.SetPos((double)(100 * params[kChorusDelay]));
		slider_depthms.SetPos((double)(100 * params[kChorusDepth]));
		slider_freq.SetPos((double)(100 * params[kChorusFrequency]));
		slider_drywet.SetPos((double)(100 * params[kChorusWetDryMix]));
		slider_feedback.SetPos((double)(100 * params[kChorusFeedback]));
		int shape = (int)params[kChorusWaveShape];
		::SendMessage(w, CB_SETCURSEL, shape, 0);
		int phase = ((int)round(params[kChorusPhase]));
		::SendMessage(w2, CB_SETCURSEL, phase, 0);
		RefreshLabel(params[kChorusDelay], params[kChorusDepth],
			params[kChorusFrequency], params[kChorusWetDryMix], params[kChorusFeedback]);

	}

	static uint32_t parseConfig(ui_element_config::ptr cfg) {
		return 1;

	}
	static ui_element_config::ptr makeConfig(bool init = false) {
		ui_element_config_builder out;

		if (init)
		{
			uint32_t crap = 1;
			out << crap;
		}
		else
		{
			uint32_t crap = 2;
			out << crap;
		}
		return out.finish(g_get_guid());
	}

	BOOL OnInitDialog(CWindow, LPARAM) {
		CWindow w = GetDlgItem(IDC_DMOCHORUSWAVETYPE2);
		CWindow w2 = GetDlgItem(IDC_DMOCHORUSPHASE2);
		uSendMessageText(w, CB_ADDSTRING, 0, "Triangle");
		uSendMessageText(w, CB_ADDSTRING, 0, "Sine");

		uSendMessageText(w2, CB_ADDSTRING, 0, "-180");
		uSendMessageText(w2, CB_ADDSTRING, 0, "-90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "0");
		uSendMessageText(w2, CB_ADDSTRING, 0, "90");
		uSendMessageText(w2, CB_ADDSTRING, 0, "180");

		m_buttonEchoEnabled = GetDlgItem(IDC_DMOCHORUSENABLE);
		m_buttonEchoEnabled.ShowWindow(SW_SHOW);


		slider_delayms = GetDlgItem(IDC_DMOCHORUSDELAYMS2);
		slider_delayms.SetRange(FreqMin, FreqMax);
		slider_depthms = GetDlgItem(IDC_DMOCHORUSDEPTHMS2);
		slider_depthms.SetRange(10, 100);
		slider_freq = GetDlgItem(IDC_DMOCHORUSLFOFREQ2);
		slider_freq.SetRange(0, 1000);
		slider_drywet = GetDlgItem(IDC_DMOCHORUSDRYWET2);
		slider_drywet.SetRange(0, 100);
		slider_feedback = GetDlgItem(IDC_DMOCHORUSFEEDBACK2);
		slider_feedback.SetRange(0, 100);


		float params2[kChorusNumParameters] =
		{
			0.5f,
			0.2f,
			0.11f,
			1.0f,
			0.75f,
			(40.0f + 99.0f) / 198.0f,
			0.8f
		};
		pfc::copy_array_loop_t(params, params2, kChorusNumParameters);

		m_buttonEchoEnabled = GetDlgItem(IDC_DMOCHORUSENABLE);
		m_ownEchoUpdate = false;
		ApplySettings();
		return FALSE;
	}


	uint32_t shit;

	void RefreshLabel(float delay_ms, float depth_ms, float lfo_freq, float drywet, float feedback)
	{
		pfc::string_formatter msg;
		msg << "Delay: ";
		msg << pfc::format_float(delay_ms, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDELAYLAB2, msg);
		msg.reset();
		msg << "Depth: ";
		msg << pfc::format_int(100 * depth_ms) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDEPTHMSLAB2, msg);
		msg.reset();
		msg << "LFO Frequency: ";
		msg << pfc::format_float(lfo_freq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DMOCHORUSLFOFREQLAB2, msg);
		msg.reset();
		msg << "Wet/Dry Mix : ";
		msg << pfc::format_int(100 * drywet) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSDRYWETLAB2, msg);
		msg.reset();
		msg << "Feedback : ";
		msg << pfc::format_int(100 * feedback) << " %";
		::uSetDlgItemText(*this, IDC_DMOCHORUSFEEDBACKLAB2, msg);

	}

	bool m_ownEchoUpdate;
	float params[kChorusNumParameters];
	CTrackBarCtrl slider_drywet, slider_depthms, slider_freq, slider_feedback, slider_delayms;
	CButton m_buttonEchoEnabled;
	bool echo_enabled;
protected:
	const ui_element_instance_callback::ptr m_callback;
};

class myElem_t2 : public  ui_element_impl_withpopup< uielem_chorus > {
	bool get_element_group(pfc::string_base& p_out)
	{
		p_out = "Effect DSP";
		return true;
	}

	bool get_menu_command_description(pfc::string_base& out) {
		out = "Opens a window for chorus effects.";
		return true;
	}

};
static service_factory_single_t<myElem_t2> g_myElemFactory;

static void RunConfigPopupChorus(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
{
	CMyDSPPopupChorus popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}

static dsp_factory_t<dsp_dmo_chorus> g_dsp_chorus_factory;

static void RunConfigPopupCompressor(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback);
class dsp_dmo_compressor : public dsp_impl_base
{
	enum Parameters
	{
		kCompGain = 0,
		kCompAttack,
		kCompRelease,
		kCompThreshold,
		kCompRatio,
		kCompPredelay,
		kCompNumParameters
	};
	float m_param[kCompNumParameters];
	int m_rate, m_ch, m_ch_mask;
	bool enabled;
	DMOFilter dmo_filt;
public:
	static GUID g_get_guid()
	{
		// {B904DAC3-5DAE-4E9B-96B5-3D92BE33BF38}
		return guid_dmo_compressor;
	}

	dsp_dmo_compressor(dsp_preset const& in) :m_rate(0), m_ch(0), m_ch_mask(0)
	{
		parse_preset(m_param, enabled, in);
		enabled = true;
	}
	~dsp_dmo_compressor()
	{
		dmo_filt.deinit();
	}

	static void g_get_name(pfc::string_base& p_out) { p_out = "Compressor (DirectX)"; }

	bool on_chunk(audio_chunk* chunk, abort_callback&)
	{
		if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			dmo_filt.deinit();
			if (m_ch <= 2)
			{
				dmo_filt.init(m_rate, DMO_Type::COMPRESSOR, m_ch);
				for (int i = 0; i < kCompNumParameters; i++)
					dmo_filt.setparameter(i, m_param[i]);
			}

		}
		if (m_ch <= 2)
		{
			audio_sample* data = chunk->get_data();
			unsigned count = chunk->get_data_size();
			dmo_filt.Process(data, count);
		}


		return true;
	}

	void on_endofplayback(abort_callback&) { }
	void on_endoftrack(abort_callback&) { }

	void flush()
	{
		dmo_filt.deinit();
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

	static bool g_get_default_preset(dsp_preset & p_out)
	{
		float params[kCompNumParameters] =
		{
	   0.5f,
	   0.02f,
	  150.0f / 2950.0f,
	  2.0f / 3.0f,
	  0.02f,
	  1.0f
		};


		make_preset(params, true, p_out);
		return true;
	}
	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		::RunConfigPopupCompressor(p_data, p_parent, p_callback);
	}
	static bool g_have_config_popup() { return true; }

	static void make_preset(float params[kCompNumParameters], bool enabled, dsp_preset & out)
	{
		dsp_preset_builder builder;
		for (int i = 0; i < kCompNumParameters; i++)
			builder << params[i];
		builder << enabled;
		builder.finish(g_get_guid(), out);
	}

	static void parse_preset(float* params, bool& enabled, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			for (int i = 0; i < kCompNumParameters; i++)
			{
				parser >> params[i];
			}
			parser >> enabled;
		}
		catch (exception_io_data) {
			params[kCompGain] = 0.5f;
			params[kCompAttack] = 0.02f;
			params[kCompRelease] = 150.0f / 2950.0f;
			params[kCompThreshold] = 2.0f / 3.0f;
			params[kCompRatio] = 0.02f;
			params[kCompPredelay] = 1.0f;
			enabled = true;
		}
	}
};

static dsp_factory_t<dsp_dmo_compressor> g_dsp_compressor_factory;

class CMyDSPPopupCompressor : public CDialogImpl<CMyDSPPopupCompressor>
{
public:
	CMyDSPPopupCompressor(const dsp_preset& initData, dsp_preset_edit_callback& callback) : m_initData(initData), m_callback(callback) { }
	enum { IDD = IDD_DMOCOMPRESSOR };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kCompGain = 0,
		kCompAttack,
		kCompRelease,
		kCompThreshold,
		kCompRatio,
		kCompPredelay,
		kCompNumParameters
	};

	BEGIN_MSG_MAP(CMyDSPPopupCompressor)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{

		slider_gain = GetDlgItem(IDC_COMPRESSORGAIN);
		slider_gain.SetRange(0, 100);
		slider_attack = GetDlgItem(IDC_COMPRESSORATTACK);
		slider_attack.SetRange(0, 100);
		slider_release = GetDlgItem(IDC_COMPRESSORRELEASE);
		slider_release.SetRange(0, 100);
		slider_threshold = GetDlgItem(IDC_COMPRESSORTHRESHOLD);
		slider_threshold.SetRange(0, 100);
		slider_ratio = GetDlgItem(IDC_COMPRESSORRATIO);
		slider_ratio.SetRange(0, 100);
		slider_predelay = GetDlgItem(IDC_COMPRESSORPREDELAY);
		slider_predelay.SetRange(0, 100);

		bool enabled;
		dsp_dmo_compressor::parse_preset(params, enabled, m_initData);
		slider_gain.SetPos((double)(100 * params[kCompGain]));
		slider_attack.SetPos((double)(100 * params[kCompAttack]));
		slider_release.SetPos((double)(100 * params[kCompRelease]));
		slider_threshold.SetPos((double)(100 * params[kCompThreshold]));
		slider_ratio.SetPos((double)(100 * params[kCompRatio]));
		slider_predelay.SetPos((double)(100 * params[kCompPredelay]));
		float gain = -60.0f + params[kCompGain] * 120.0f;
		float attack = 0.01f + params[kCompAttack] * 499.99f;
		float release = 50.0f + params[kCompRelease] * 2950.0f;
		float thres = -60.0f + params[kCompThreshold] * 60.0f;
		float ratio = 1.0f + params[kCompRatio] * 99.0f;
		float predelay = params[kCompPredelay] * 4.0f;
		RefreshLabel(gain, attack, release, thres, ratio, predelay);
		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnChange(UINT, int id, CWindow)
	{
		params[kCompGain] = slider_gain.GetPos() / 100.0;
		params[kCompAttack] = slider_attack.GetPos() / 100.0;
		params[kCompRelease] = slider_release.GetPos() / 100.0;
		params[kCompThreshold] = slider_threshold.GetPos() / 100.0;
		params[kCompRatio] = slider_ratio.GetPos() / 100.0;
		params[kCompPredelay] = slider_predelay.GetPos() / 100.0;


		dsp_preset_impl preset;
		dsp_dmo_compressor::make_preset(params, true, preset);
		m_callback.on_preset_changed(preset);
		float gain = -60.0f + params[kCompGain] * 120.0f;
		float attack = 0.01f + params[kCompAttack] * 499.99f;
		float release = 50.0f + params[kCompRelease] * 2950.0f;
		float thres = -60.0f + params[kCompThreshold] * 60.0f;
		float ratio = 1.0f + params[kCompRatio] * 99.0f;
		float predelay = params[kCompPredelay] * 4.0f;

		RefreshLabel(gain, attack, release, thres, ratio, predelay);

	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		params[kCompGain] = slider_gain.GetPos() / 100.0;
		params[kCompAttack] = slider_attack.GetPos() / 100.0;
		params[kCompRelease] = slider_release.GetPos() / 100.0;
		params[kCompThreshold] = slider_threshold.GetPos() / 100.0;
		params[kCompRatio] = slider_ratio.GetPos() / 100.0;
		params[kCompPredelay] = slider_predelay.GetPos() / 100.0;

		if (LOWORD(nSBCode) != SB_THUMBTRACK)
		{
			dsp_preset_impl preset;
			dsp_dmo_compressor::make_preset(params, true, preset);
			m_callback.on_preset_changed(preset);
		}
		float gain = -60.0f + params[kCompGain] * 120.0f;
		float attack = 0.01f + params[kCompAttack] * 499.99f;
		float release = 50.0f + params[kCompRelease] * 2950.0f;
		float thres = -60.0f + params[kCompThreshold] * 60.0f;
		float ratio = 1.0f + params[kCompRatio] * 99.0f;
		float predelay = params[kCompPredelay] * 4.0f;

		RefreshLabel(gain, attack, release, thres, ratio, predelay);

	}

	void RefreshLabel(float gain_ms, float attack, float release, float thres, float ratio, float predelay)
	{
		pfc::string_formatter msg;
		msg << "Gain: ";
		msg << pfc::format_float(gain_ms, 0, 2) << " db";
		::uSetDlgItemText(*this, IDC_COMPRESSORGAINLAB, msg);
		msg.reset();
		msg << "Attack: ";
		msg << pfc::format_float(attack, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_COMPRESSORATTACKLAB, msg);
		msg.reset();
		msg << "Release: ";
		msg << pfc::format_float(release, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_COMPRESSORRELEASELAB, msg);
		msg.reset();
		msg << "Threshold : ";
		msg << pfc::format_float(thres, 0, 2) << " db";
		::uSetDlgItemText(*this, IDC_COMPRESSORTHRESHOLDLAB, msg);
		msg.reset();
		msg << "Ratio : ";
		msg << pfc::format_float(ratio, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_COMPRESSORRATIOLAB, msg);
		msg.reset();
		msg << "Predelay : ";
		msg << pfc::format_float(predelay, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_COMPRESSORPREDELAYLAB, msg);
	}

	const dsp_preset& m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback& m_callback;
	float params[kCompNumParameters];
	CTrackBarCtrl slider_gain, slider_attack, slider_release, slider_threshold, slider_ratio, slider_predelay;
};

static void RunConfigPopupCompressor(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback)
{
	CMyDSPPopupCompressor popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}

// {042F153B-B5F2-44DE-94D7-170E4C491BFA}
static const GUID guid_compressorelem =
{ 0x42f153b, 0xb5f2, 0x44de, { 0x94, 0xd7, 0x17, 0xe, 0x4c, 0x49, 0x1b, 0xfa } };


class uielem_compressor : public CDialogImpl<uielem_compressor>, public ui_element_instance {
public:
	uielem_compressor(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
		echo_enabled = true;
	}
	enum { IDD = IDD_DMOCOMPRESSOR1 };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kCompGain = 0,
		kCompAttack,
		kCompRelease,
		kCompThreshold,
		kCompRatio,
		kCompPredelay,
		kCompNumParameters
	};

	BEGIN_MSG_MAP(uielem_compressor)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_COMPRESSENABLE, BN_CLICKED, OnEnabledToggle)
		MSG_WM_HSCROLL(OnScroll)
	END_MSG_MAP()

	void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
	HWND get_wnd() { return m_hWnd; }
	void set_configuration(ui_element_config::ptr config) {
		shit = parseConfig(config);
		if (m_hWnd != NULL) {
			ApplySettings();
		}
		m_callback->on_min_max_info_change();
	}
	ui_element_config::ptr get_configuration() { return makeConfig(); }
	static GUID g_get_guid() {
		return guid_compressorelem;
	}
	static void g_get_name(pfc::string_base & out) { out = "Compressor (DirectX)"; }
	static ui_element_config::ptr g_get_default_configuration() {
		return makeConfig(true);
	}
	static const char* g_get_description() { return "Modifies the 'Compressor' DSP effect."; }
	static GUID g_get_subclass() {
		return ui_element_subclass_dsp;
	}

	ui_element_min_max_info get_min_max_info() {
		ui_element_min_max_info ret;

		// Note that we play nicely with separate horizontal & vertical DPI.
		// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
		CSize DPI = QueryScreenDPIEx(*this);

		if (DPI.cx <= 0 || DPI.cy <= 0) { // sanity
			DPI = CSize(96, 96);
		}


		ret.m_min_width = MulDiv(440, DPI.cx, 96);
		ret.m_min_height = MulDiv(220, DPI.cy, 96);
		ret.m_max_width = MulDiv(440, DPI.cx, 96);
		ret.m_max_height = MulDiv(220, DPI.cy, 96);

		// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
		ret.adjustForWindow(*this);

		return ret;
	}

private:
	uint32_t shit;

	static uint32_t parseConfig(ui_element_config::ptr cfg) {
		return 1;

	}
	static ui_element_config::ptr makeConfig(bool init = false) {
		ui_element_config_builder out;

		if (init)
		{
			uint32_t crap = 1;
			out << crap;
		}
		else
		{
			uint32_t crap = 2;
			out << crap;
		}
		return out.finish(g_get_guid());
	}

	void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
	bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

	void EchoDisable() {
		static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_compressor);
	}

	void EchoEnable(float params[], bool echo_enabled) {
		dsp_preset_impl preset;
		dsp_dmo_compressor::make_preset(params, echo_enabled, preset);
		static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
	}

	void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		if (IsEchoEnabled()) {
			GetConfig();
			dsp_preset_impl preset;
			dsp_dmo_compressor::make_preset(params, echo_enabled, preset);
			//yes change api;
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}
		else {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_compressor);
		}

	}

	void ApplySettings()
	{
		dsp_preset_impl preset;
		if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_dmo_compressor, preset)) {
			SetEchoEnabled(true);
			dsp_dmo_compressor::parse_preset(params, echo_enabled, preset);
			SetEchoEnabled(echo_enabled);
			SetConfig();
		}
		else {
			SetEchoEnabled(false);
			SetConfig();
		}
	}


	void GetConfig()
	{
		params[kCompGain] = slider_gain.GetPos() / 1000.0;
		params[kCompAttack] = slider_attack.GetPos() / 1000.0;
		params[kCompRelease] = slider_release.GetPos() / 1000.0;
		params[kCompThreshold] = slider_threshold.GetPos() / 1000.0;
		params[kCompRatio] = slider_ratio.GetPos() / 100.0;
		params[kCompPredelay] = slider_predelay.GetPos() / 1000.0;
		echo_enabled = IsEchoEnabled();

		float gain = -60.0f + params[kCompGain] * 120.0f;
		float attack = 0.01f + params[kCompAttack] * 499.99f;
		float release = 50.0f + params[kCompRelease] * 2950.0f;
		float thres = -60.0f + params[kCompThreshold] * 60.0f;
		float ratio = 1.0f + params[kCompRatio] * 99.0f;
		float predelay = params[kCompPredelay] * 4.0f;

		RefreshLabel(gain, attack, release, thres, ratio, predelay);
	}

	void SetConfig()
	{


		slider_gain.SetPos((double)(1000 * params[kCompGain]));
		slider_attack.SetPos((double)(1000 * params[kCompAttack]));
		slider_release.SetPos((double)(1000 * params[kCompRelease]));
		slider_threshold.SetPos((double)(1000 * params[kCompThreshold]));
		slider_ratio.SetPos((double)(100 * params[kCompRatio]));
		slider_predelay.SetPos((double)(1000 * params[kCompPredelay]));

		float gain = -60.0f + params[kCompGain] * 120.0f;
		float attack = 0.01f + params[kCompAttack] * 499.99f;
		float release = 50.0f + params[kCompRelease] * 2950.0f;
		float thres = -60.0f + params[kCompThreshold] * 60.0f;
		float ratio = 1.0f + params[kCompRatio] * 99.0f;
		float predelay = params[kCompPredelay] * 4.0f;

		RefreshLabel(gain, attack, release, thres, ratio, predelay);
	}


	BOOL OnInitDialog(CWindow, LPARAM)
	{

		modeless_dialog_manager::g_add(m_hWnd);
		m_buttonEchoEnabled = GetDlgItem(IDC_COMPRESSENABLE);
		m_buttonEchoEnabled.ShowWindow(SW_SHOW);


		slider_gain = GetDlgItem(IDC_COMPRESSORGAIN2);
		slider_gain.SetRange(0, 1000);
		slider_attack = GetDlgItem(IDC_COMPRESSORATTACK2);
		slider_attack.SetRange(0, 1000);
		slider_release = GetDlgItem(IDC_COMPRESSORRELEASE2);
		slider_release.SetRange(0, 1000);
		slider_threshold = GetDlgItem(IDC_COMPRESSORTHRESHOLD2);
		slider_threshold.SetRange(0, 1000);
		slider_ratio = GetDlgItem(IDC_COMPRESSORRATIO2);
		slider_ratio.SetRange(0, 100);
		slider_predelay = GetDlgItem(IDC_COMPRESSORPREDELAY2);
		slider_predelay.SetRange(0, 1000);


		float params2[kCompNumParameters] =
		{
		0.5f,
		0.02f,
	   150.0f / 2950.0f,
		2.0f / 3.0f,
		0.02f,
		1.0f
		};
		pfc::copy_array_loop_t(params, params2, kCompNumParameters);

		ApplySettings();
		return TRUE;
	}

	void OnConfigChanged() {
		if (IsEchoEnabled()) {
			EchoEnable(params, echo_enabled);
		}
		else {
			EchoDisable();
		}

	}

	void OnButton(UINT, int id, CWindow)
	{
		DestroyWindow();
	}


	void OnScroll(UINT scrollID, int pos, CWindow window)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			if (LOWORD(scrollID) != SB_THUMBTRACK)
			{
				EchoEnable(params, echo_enabled);
			}
		}

	}

	void OnChange(UINT, int id, CWindow)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			OnConfigChanged();
		}
	}

	void RefreshLabel(float gain_ms, float attack, float release, float thres, float ratio, float predelay)
	{
		pfc::string_formatter msg;
		msg << "Gain: ";
		msg << pfc::format_float(gain_ms, 0, 2) << " db";
		::uSetDlgItemText(*this, IDC_COMPRESSORGAINLAB2, msg);
		msg.reset();
		msg << "Attack: ";
		msg << pfc::format_float(attack, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_COMPRESSORATTACKLAB2, msg);
		msg.reset();
		msg << "Release: ";
		msg << pfc::format_float(release, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_COMPRESSORRELEASELAB2, msg);
		msg.reset();
		msg << "Threshold : ";
		msg << pfc::format_float(thres, 0, 2) << " db";
		::uSetDlgItemText(*this, IDC_COMPRESSORTHRESHOLDLAB2, msg);
		msg.reset();
		msg << "Ratio : ";
		msg << pfc::format_float(ratio, 0, 2) << " %";
		::uSetDlgItemText(*this, IDC_COMPRESSORRATIOLAB2, msg);
		msg.reset();
		msg << "Predelay : ";
		msg << pfc::format_float(predelay, 0, 2) << " ms";
		::uSetDlgItemText(*this, IDC_COMPRESSORPREDELAYLAB2, msg);

	}
	bool m_ownEchoUpdate;
	float params[kCompNumParameters];
	CTrackBarCtrl slider_gain, slider_attack, slider_release, slider_threshold, slider_ratio, slider_predelay;
	CButton m_buttonEchoEnabled;
	bool echo_enabled;
protected:
	const ui_element_instance_callback::ptr m_callback;
};

class myElem_t3 : public  ui_element_impl_withpopup< uielem_compressor > {
	bool get_element_group(pfc::string_base& p_out)
	{
		p_out = "Effect DSP";
		return true;
	}

	bool get_menu_command_description(pfc::string_base& out) {
		out = "Opens a window for dynamics compression.";
		return true;
	}

};
static service_factory_single_t<myElem_t3> g_myElemFactory2;

static void RunConfigPopupDistort(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback);
class dsp_dmo_distort : public dsp_impl_base
{
	enum Parameters
	{
		kDistGain = 0,
		kDistEdge,
		kDistPreLowpassCutoff,
		kDistPostEQCenterFrequency,
		kDistPostEQBandwidth,
		kDistNumParameters
	};
	float m_param[kDistNumParameters];
	int m_rate, m_ch, m_ch_mask;
	bool enabled;
	DMOFilter dmo_filt;
public:
	static GUID g_get_guid()
	{
		// {B904DAC3-5DAE-4E9B-96B5-3D92BE33BF38}
		return guid_dmo_distort;
	}

	dsp_dmo_distort(dsp_preset const& in) :m_rate(0), m_ch(0), m_ch_mask(0)
	{
		parse_preset(m_param, enabled, in);
		enabled = true;
	}
	~dsp_dmo_distort()
	{
		dmo_filt.deinit();
	}

	static void g_get_name(pfc::string_base& p_out) { p_out = "Distortion (DirectX)"; }

	bool on_chunk(audio_chunk* chunk, abort_callback&)
	{
		if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			dmo_filt.deinit();
			if (m_ch <= 2)
			{
				dmo_filt.init(m_rate, DMO_Type::DISTORTION, m_ch);
				for (int i = 0; i < kDistNumParameters; i++)
					dmo_filt.setparameter(i, m_param[i]);
			}

		}
		if (m_ch <= 2)
		{
			audio_sample* data = chunk->get_data();
			unsigned count = chunk->get_data_size();
			dmo_filt.Process(data, count);
		}


		return true;
	}

	void on_endofplayback(abort_callback&) { }
	void on_endoftrack(abort_callback&) { }

	void flush()
	{
		dmo_filt.deinit();
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

	static bool g_get_default_preset(dsp_preset & p_out)
	{
		float params[kDistNumParameters] =
		{
	   0.7f,
	   0.15f,
	  1.0f,
	  0.29f,
	  0.29
		};


		make_preset(params, true, p_out);
		return true;
	}
	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		::RunConfigPopupDistort(p_data, p_parent, p_callback);
	}
	static bool g_have_config_popup() { return true; }

	static void make_preset(float params[kDistNumParameters], bool enabled, dsp_preset & out)
	{
		dsp_preset_builder builder;
		for (int i = 0; i < kDistNumParameters; i++)
			builder << params[i];
		builder << enabled;
		builder.finish(g_get_guid(), out);
	}

	static void parse_preset(float* params, bool& enabled, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			for (int i = 0; i < kDistNumParameters; i++)
			{
				parser >> params[i];
			}
			parser >> enabled;
		}
		catch (exception_io_data) {

			params[kDistGain] = 0.7f;
			params[kDistEdge] = 0.15f;
			params[kDistPreLowpassCutoff] = 1.0f;
			params[kDistPostEQCenterFrequency] = 0.291f;
			params[kDistPostEQBandwidth] = 0.291f;
			enabled = true;
		}
	}
};

static dsp_factory_t<dsp_dmo_distort> g_dsp_distort_factory;

class CMyDSPPopupDistort : public CDialogImpl<CMyDSPPopupDistort>
{
public:
	CMyDSPPopupDistort(const dsp_preset& initData, dsp_preset_edit_callback& callback) : m_initData(initData), m_callback(callback) { }
	enum { IDD = IDD_DMODISTORTION };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kDistGain = 0,
		kDistEdge,
		kDistPreLowpassCutoff,
		kDistPostEQCenterFrequency,
		kDistPostEQBandwidth,
		kDistNumParameters
	};

	BEGIN_MSG_MAP(CMyDSPPopupDistort)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		slider_gain = GetDlgItem(IDC_DISTORTIONGAIN);
		slider_gain.SetRange(0, 100);
		slider_edge = GetDlgItem(IDC_DISTORTIONEDGE);
		slider_edge.SetRange(0, 100);
		slider_prelopass = GetDlgItem(IDC_DISTORTIONPRELOPASSCUTOFF);
		slider_prelopass.SetRange(0, 100);
		slider_posteqcentrefreq = GetDlgItem(IDC_DISTORTIONPOSTEQCENTERFREQ);
		slider_posteqcentrefreq.SetRange(0, 100);
		slider_posteqbandwidth = GetDlgItem(IDC_DISTORTIONPOSTEQBANDWIDTH);
		slider_posteqbandwidth.SetRange(0, 100);

		bool enabled;
		dsp_dmo_distort::parse_preset(params, enabled, m_initData);
		slider_gain.SetPos((double)(100 * params[kDistGain]));
		slider_edge.SetPos((double)(100 * params[kDistEdge]));
		slider_prelopass.SetPos((double)(100 * params[kDistPreLowpassCutoff]));
		slider_posteqcentrefreq.SetPos((double)(100 * params[kDistPostEQCenterFrequency]));
		slider_posteqbandwidth.SetPos((double)(100 * params[kDistPostEQBandwidth]));


		float gain = -60.0f + params[kDistGain] * 60.0f;
		float edge = params[kDistEdge] * 100.f;
		float prelowpass = 100.0f + params[kDistPreLowpassCutoff] * 7900.0f;
		float postcentrefreq = 100.0f + params[kDistPostEQCenterFrequency] * 7900.0f;
		float posteqbandwith = 100.0f + params[kDistPostEQBandwidth] * 7900.0f;
		RefreshLabel(gain, edge, prelowpass, postcentrefreq, posteqbandwith);
		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnChange(UINT, int id, CWindow)
	{
		params[kDistGain] = slider_gain.GetPos() / 100.0;
		params[kDistEdge] = slider_edge.GetPos() / 100.0;
		params[kDistPreLowpassCutoff] = slider_prelopass.GetPos() / 100.0;
		params[kDistPostEQCenterFrequency] = slider_posteqcentrefreq.GetPos() / 100.0;
		params[kDistPostEQBandwidth] = slider_posteqbandwidth.GetPos() / 100.0;


		dsp_preset_impl preset;
		dsp_dmo_distort::make_preset(params, true, preset);
		m_callback.on_preset_changed(preset);
		

		float gain = -60.0f + params[kDistGain] * 60.0f;
		float edge = params[kDistEdge] * 100.f;
		float prelowpass = 100.0f + params[kDistPreLowpassCutoff] * 7900.0f;
		float postcentrefreq = 100.0f + params[kDistPostEQCenterFrequency] * 7900.0f;
		float posteqbandwith = 100.0f + params[kDistPostEQBandwidth] * 7900.0f;

		RefreshLabel(gain, edge, prelowpass, postcentrefreq, posteqbandwith);
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		params[kDistGain] = slider_gain.GetPos() / 100.0;
		params[kDistEdge] = slider_edge.GetPos() / 100.0;
		params[kDistPreLowpassCutoff] = slider_prelopass.GetPos() / 100.0;
		params[kDistPostEQCenterFrequency] = slider_posteqcentrefreq.GetPos() / 100.0;
		params[kDistPostEQBandwidth] = slider_posteqbandwidth.GetPos() / 100.0;

		if (LOWORD(nSBCode) != SB_THUMBTRACK)
		{
			dsp_preset_impl preset;
			dsp_dmo_distort::make_preset(params, true, preset);
			m_callback.on_preset_changed(preset);
		}
		float gain = -60.0f + params[kDistGain] * 60.0f;
		float edge = params[kDistEdge] * 100.f;
		float prelowpass = 100.0f + params[kDistPreLowpassCutoff] * 7900.0f;
		float postcentrefreq = 100.0f + params[kDistPostEQCenterFrequency] * 7900.0f;
		float posteqbandwith = 100.0f + params[kDistPostEQBandwidth] * 7900.0f;

		RefreshLabel(gain, edge, prelowpass, postcentrefreq, posteqbandwith);

	}

	void RefreshLabel(float gain, float edge, float prelowpass, float postcentrefreq, float posteqbandwidth)
	{
		pfc::string_formatter msg;
		msg << "Gain: ";
		msg << pfc::format_float(gain, 0, 2) << " db";
		::uSetDlgItemText(*this, IDC_DISTORTIONGAINLAB, msg);
		msg.reset();
		msg << "Edge: ";
		msg << pfc::format_float(edge, 0, 2) << " %";
		::uSetDlgItemText(*this, IDC_DISTORTIONEDGELAB, msg);
		msg.reset();
		msg << "Pre-lowpass cutoff: ";
		msg << pfc::format_float(prelowpass, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DISTORTIONCUTOFFLAB, msg);
		msg.reset();
		msg << "Post-EQ Centre Freq : ";
		msg << pfc::format_float(postcentrefreq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DISTORTIONCENTFREQ, msg);
		msg.reset();
		msg << "Post-EQ Bandwidth : ";
		msg << pfc::format_float(posteqbandwidth, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DISTORTIONEPOSTEQBW, msg);
		msg.reset();
	}

	const dsp_preset& m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback& m_callback;
	float params[kDistNumParameters];
	CTrackBarCtrl slider_gain, slider_edge, slider_prelopass, slider_posteqcentrefreq, slider_posteqbandwidth;
};

static void RunConfigPopupDistort(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback)
{
	CMyDSPPopupDistort popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}

// {042F153B-B5F2-44DE-94D7-170E4C491BFA}
static const GUID guid_distortelem =
{ 0x3ac3a59a, 0x3ea3, 0x4b88, { 0xa1, 0xd0, 0x97, 0xff, 0x9c, 0xba, 0xdd, 0x83 } };



class uielem_distort : public CDialogImpl<uielem_distort>, public ui_element_instance {
public:
	uielem_distort(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
		echo_enabled = true;
	}
	enum { IDD = IDD_DMODISTORTION1 };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kDistGain = 0,
		kDistEdge,
		kDistPreLowpassCutoff,
		kDistPostEQCenterFrequency,
		kDistPostEQBandwidth,
		kDistNumParameters
	};

	BEGIN_MSG_MAP(uielem_distort)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_DISTORTENABLE, BN_CLICKED, OnEnabledToggle)
		MSG_WM_HSCROLL(OnScroll)
	END_MSG_MAP()

	void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
	HWND get_wnd() { return m_hWnd; }
	void set_configuration(ui_element_config::ptr config) {
		shit = parseConfig(config);
		if (m_hWnd != NULL) {
			ApplySettings();
		}
		m_callback->on_min_max_info_change();
	}
	ui_element_config::ptr get_configuration() { return makeConfig(); }
	static GUID g_get_guid() {
		return guid_distortelem;
	}
	static void g_get_name(pfc::string_base & out) { out = "Distortion (DirectX)"; }
	static ui_element_config::ptr g_get_default_configuration() {
		return makeConfig(true);
	}
	static const char* g_get_description() { return "Modifies the 'Distortion' DSP effect."; }
	static GUID g_get_subclass() {
		return ui_element_subclass_dsp;
	}

	ui_element_min_max_info get_min_max_info() {
		ui_element_min_max_info ret;

		// Note that we play nicely with separate horizontal & vertical DPI.
		// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
		CSize DPI = QueryScreenDPIEx(*this);

		if (DPI.cx <= 0 || DPI.cy <= 0) { // sanity
			DPI = CSize(96, 96);
		}


		ret.m_min_width = MulDiv(440, DPI.cx, 96);
		ret.m_min_height = MulDiv(220, DPI.cy, 96);
		ret.m_max_width = MulDiv(440, DPI.cx, 96);
		ret.m_max_height = MulDiv(220, DPI.cy, 96);

		// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
		ret.adjustForWindow(*this);

		return ret;
	}

private:
	uint32_t shit;

	static uint32_t parseConfig(ui_element_config::ptr cfg) {
		return 1;

	}
	static ui_element_config::ptr makeConfig(bool init = false) {
		ui_element_config_builder out;

		if (init)
		{
			uint32_t crap = 1;
			out << crap;
		}
		else
		{
			uint32_t crap = 2;
			out << crap;
		}
		return out.finish(g_get_guid());
	}

	void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
	bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

	void EchoDisable() {
		static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_distort);
	}

	void EchoEnable(float params[], bool echo_enabled) {
		dsp_preset_impl preset;
		dsp_dmo_distort::make_preset(params, echo_enabled, preset);
		static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
	}

	void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		if (IsEchoEnabled()) {
			GetConfig();
			dsp_preset_impl preset;
			dsp_dmo_distort::make_preset(params, echo_enabled, preset);
			//yes change api;
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}
		else {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_distort);
		}

	}

	void ApplySettings()
	{
		dsp_preset_impl preset;
		if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_dmo_distort, preset)) {
			SetEchoEnabled(true);
			dsp_dmo_distort::parse_preset(params, echo_enabled, preset);
			SetEchoEnabled(echo_enabled);
			SetConfig();
		}
		else {
			SetEchoEnabled(false);
			SetConfig();
		}
	}


	void GetConfig()
	{
		params[kDistGain] = slider_gain.GetPos() / 100.0;
		params[kDistEdge] = slider_edge.GetPos() / 100.0;
		params[kDistPreLowpassCutoff] = slider_prelopass.GetPos() / 100.0;
		params[kDistPostEQCenterFrequency] = slider_posteqcentrefreq.GetPos() / 100.0;
		params[kDistPostEQBandwidth] = slider_posteqbandwidth.GetPos() / 100.0;
		echo_enabled = IsEchoEnabled();

		float gain = -60.0f + params[kDistGain] * 60.0f;
		float edge = params[kDistEdge] * 100.f;
		float prelowpass = 100.0f + params[kDistPreLowpassCutoff] * 7900.0f;
		float postcentrefreq = 100.0f + params[kDistPostEQCenterFrequency] * 7900.0f;
		float posteqbandwith = 100.0f + params[kDistPostEQBandwidth] * 7900.0f;

		RefreshLabel(gain, edge, prelowpass, postcentrefreq, posteqbandwith);
	}

	void SetConfig()
	{
		slider_gain.SetPos((double)(100 * params[kDistGain]));
		slider_edge.SetPos((double)(100 * params[kDistEdge]));
		slider_prelopass.SetPos((double)(100 * params[kDistPreLowpassCutoff]));
		slider_posteqcentrefreq.SetPos((double)(100 * params[kDistPostEQCenterFrequency]));
		slider_posteqbandwidth.SetPos((double)(100 * params[kDistPostEQBandwidth]));


		float gain = -60.0f + params[kDistGain] * 60.0f;
		float edge = params[kDistEdge] * 100.f;
		float prelowpass = 100.0f + params[kDistPreLowpassCutoff] * 7900.0f;
		float postcentrefreq = 100.0f + params[kDistPostEQCenterFrequency] * 7900.0f;
		float posteqbandwith = 100.0f + params[kDistPostEQBandwidth] * 7900.0f;
		RefreshLabel(gain, edge, prelowpass, postcentrefreq, posteqbandwith);
	}


	BOOL OnInitDialog(CWindow, LPARAM)
	{

		modeless_dialog_manager::g_add(m_hWnd);
		m_buttonEchoEnabled = GetDlgItem(IDC_DISTORTENABLE);
		m_buttonEchoEnabled.ShowWindow(SW_SHOW);


		slider_gain = GetDlgItem(IDC_DISTORTIONGAIN);
		slider_gain.SetRange(0, 100);
		slider_edge = GetDlgItem(IDC_DISTORTIONEDGE);
		slider_edge.SetRange(0, 100);
		slider_prelopass = GetDlgItem(IDC_DISTORTIONPRELOPASSCUTOFF);
		slider_prelopass.SetRange(0, 100);
		slider_posteqcentrefreq = GetDlgItem(IDC_DISTORTIONPOSTEQCENTERFREQ);
		slider_posteqcentrefreq.SetRange(0, 100);
		slider_posteqbandwidth = GetDlgItem(IDC_DISTORTIONPOSTEQBANDWIDTH);
		slider_posteqbandwidth.SetRange(0, 100);

		float params2[kDistNumParameters] = { 0 };
		params2[kDistGain] = 0.7f;
		params2[kDistEdge] = 0.15f;
		params2[kDistPreLowpassCutoff] = 1.0f;
		params2[kDistPostEQCenterFrequency] = 0.291f;
		params2[kDistPostEQBandwidth] = 0.291f;
		pfc::copy_array_loop_t(params, params2, kDistNumParameters);

		ApplySettings();
		return TRUE;
	}

	void OnConfigChanged() {
		if (IsEchoEnabled()) {
			EchoEnable(params, echo_enabled);
		}
		else {
			EchoDisable();
		}

	}

	void OnButton(UINT, int id, CWindow)
	{
		DestroyWindow();
	}


	void OnScroll(UINT scrollID, int pos, CWindow window)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			if (LOWORD(scrollID) != SB_THUMBTRACK)
			{
				EchoEnable(params, echo_enabled);
			}
		}

	}

	void OnChange(UINT, int id, CWindow)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			OnConfigChanged();
		}
	}

	void RefreshLabel(float gain, float edge, float prelowpass, float postcentrefreq, float posteqbandwidth)
	{
		pfc::string_formatter msg;
		msg << "Gain: ";
		msg << pfc::format_float(gain, 0, 2) << " db";
		::uSetDlgItemText(*this, IDC_DISTORTIONGAINLAB, msg);
		msg.reset();
		msg << "Edge: ";
		msg << pfc::format_float(edge, 0, 2) << " %";
		::uSetDlgItemText(*this, IDC_DISTORTIONEDGELAB, msg);
		msg.reset();
		msg << "Pre-lowpass cutoff: ";
		msg << pfc::format_float(prelowpass, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DISTORTIONCUTOFFLAB, msg);
		msg.reset();
		msg << "Post-EQ Centre Freq : ";
		msg << pfc::format_float(postcentrefreq, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DISTORTIONCENTFREQLAB, msg);
		msg.reset();
		msg << "Post-EQ Bandwidth : ";
		msg << pfc::format_float(posteqbandwidth, 0, 2) << " Hz";
		::uSetDlgItemText(*this, IDC_DISTORTIONEPOSTEQBWLAB, msg);
		msg.reset();
	}
	bool m_ownEchoUpdate;
	float params[kDistNumParameters];
	CTrackBarCtrl slider_gain, slider_edge, slider_prelopass, slider_posteqcentrefreq, slider_posteqbandwidth;
	CButton m_buttonEchoEnabled;
	bool echo_enabled;
protected:
	const ui_element_instance_callback::ptr m_callback;
};

class myElem_t4 : public  ui_element_impl_withpopup< uielem_distort > {
	bool get_element_group(pfc::string_base& p_out)
	{
		p_out = "Effect DSP";
		return true;
	}

	bool get_menu_command_description(pfc::string_base& out) {
		out = "Opens a window for distortion.";
		return true;
	}

};
static service_factory_single_t<myElem_t4> g_myElemFactory3;


static void RunConfigPopupGargle(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback);
class dsp_dmo_gargle : public dsp_impl_base
{
	enum Parameters
	{
		kGargleRate = 0,
		kGargleWaveShape,
		kEqNumParameters
	};
	float m_param[kEqNumParameters];
	int m_rate, m_ch, m_ch_mask;
	bool enabled;
	DMOFilter dmo_filt;
public:
	static GUID g_get_guid()
	{
		// {B904DAC3-5DAE-4E9B-96B5-3D92BE33BF38}
		return guid_dmo_gargle;
	}

	dsp_dmo_gargle(dsp_preset const& in) :m_rate(0), m_ch(0), m_ch_mask(0)
	{
		parse_preset(m_param, enabled, in);
		enabled = true;
	}
	~dsp_dmo_gargle()
	{
		dmo_filt.deinit();
	}

	static void g_get_name(pfc::string_base& p_out) { p_out = "Gargle (DirectX)"; }

	bool on_chunk(audio_chunk* chunk, abort_callback&)
	{
		if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			dmo_filt.deinit();
			if (m_ch <= 2)
			{
				dmo_filt.init(m_rate, DMO_Type::GARGLE, m_ch);
				for (int i = 0; i < kEqNumParameters; i++)
					dmo_filt.setparameter(i, m_param[i]);
			}

		}
		if (m_ch <= 2)
		{
			audio_sample* data = chunk->get_data();
			unsigned count = chunk->get_data_size();
			dmo_filt.Process(data, count);
		}


		return true;
	}

	void on_endofplayback(abort_callback&) { }
	void on_endoftrack(abort_callback&) { }

	void flush()
	{
		dmo_filt.deinit();
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

	static bool g_get_default_preset(dsp_preset & p_out)
	{
		float params[kEqNumParameters] =
		{
	   0.02f,
	   0.0f
		};
		make_preset(params, true, p_out);
		return true;
	}
	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		::RunConfigPopupGargle(p_data, p_parent, p_callback);
	}
	static bool g_have_config_popup() { return true; }

	static void make_preset(float params[kEqNumParameters], bool enabled, dsp_preset & out)
	{
		dsp_preset_builder builder;
		for (int i = 0; i < kEqNumParameters; i++)
			builder << params[i];
		builder << enabled;
		builder.finish(g_get_guid(), out);
	}

	static void parse_preset(float* params, bool& enabled, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			for (int i = 0; i < kEqNumParameters; i++)
			{
				parser >> params[i];
			}
			parser >> enabled;
		}
		catch (exception_io_data) {

			params[kGargleRate] = 0.02f;
			params[kGargleWaveShape] = 0.0f;
			enabled = true;
		}
	}
};

static dsp_factory_t<dsp_dmo_gargle> g_dsp_gargle_factory;

class CMyDSPPopupGargle : public CDialogImpl<CMyDSPPopupGargle>
{
public:
	CMyDSPPopupGargle(const dsp_preset& initData, dsp_preset_edit_callback& callback) : m_initData(initData), m_callback(callback) { }
	enum { IDD = IDD_GARGLE };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kGargleRate = 0,
		kGargleWaveShape,
		kEqNumParameters
	};

	BEGIN_MSG_MAP(CMyDSPPopupGargle)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:

	int RateInHertz() const
	{
		return (int)(round(params[kGargleRate] * 99.0f) + 1);
	}

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		slider_rate = GetDlgItem(IDC_GARGLEFREQ);
		slider_rate.SetRange(0, 1000);
		bool enabled;
		dsp_dmo_gargle::parse_preset(params, enabled, m_initData);
		slider_rate.SetPos((double)(1000 * params[kGargleRate]));
		int gain = RateInHertz();
		RefreshLabel(gain);
		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnChange(UINT, int id, CWindow)
	{
		params[kGargleRate] = slider_rate.GetPos() / 1000.0;

		dsp_preset_impl preset;
		dsp_dmo_gargle::make_preset(params, true, preset);
		m_callback.on_preset_changed(preset);
		float gain = RateInHertz();
		RefreshLabel(gain);
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		params[kGargleRate] = slider_rate.GetPos() / 1000.0;

		if (LOWORD(nSBCode) != SB_THUMBTRACK)
		{
			dsp_preset_impl preset;
			dsp_dmo_gargle::make_preset(params, true, preset);
			m_callback.on_preset_changed(preset);
		}
		int gain = RateInHertz();
		RefreshLabel(gain);
	}

	void RefreshLabel(int gain)
	{
		pfc::string_formatter msg;
		msg << "Rate: ";
		msg << pfc::format_int(gain) << " Hz";
		::uSetDlgItemText(*this, IDC_GARGLEFREQLAB, msg);
		msg.reset();
	}

	const dsp_preset& m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback& m_callback;
	float params[kEqNumParameters];
	CTrackBarCtrl slider_rate;
};

static void RunConfigPopupGargle(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback)
{
	CMyDSPPopupGargle popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}



// {D1A3B3F0-0FF4-44D6-B06B-44FBD679A3E3}
static const GUID guid_gargleelem =
{ 0xd1a3b3f0, 0xff4, 0x44d6, { 0xb0, 0x6b, 0x44, 0xfb, 0xd6, 0x79, 0xa3, 0xe3 } };




class uielem_gargle : public CDialogImpl<uielem_gargle>, public ui_element_instance {
public:
	uielem_gargle(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
		echo_enabled = true;
	}
	enum { IDD = IDD_GARGLE1 };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kGargleRate = 0,
		kGargleWaveShape,
		kEqNumParameters
	};

	BEGIN_MSG_MAP(uielem_gargle)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_GARGLEENABLED, BN_CLICKED, OnEnabledToggle)
		MSG_WM_HSCROLL(OnScroll)
	END_MSG_MAP()

	void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
	HWND get_wnd() { return m_hWnd; }
	void set_configuration(ui_element_config::ptr config) {
		shit = parseConfig(config);
		if (m_hWnd != NULL) {
			ApplySettings();
		}
		m_callback->on_min_max_info_change();
	}
	ui_element_config::ptr get_configuration() { return makeConfig(); }
	static GUID g_get_guid() {
		return guid_gargleelem;
	}
	static void g_get_name(pfc::string_base & out) { out = "Gargle (DirectX)"; }
	static ui_element_config::ptr g_get_default_configuration() {
		return makeConfig(true);
	}
	static const char* g_get_description() { return "Modifies the 'Gargle' DSP effect."; }
	static GUID g_get_subclass() {
		return ui_element_subclass_dsp;
	}

	ui_element_min_max_info get_min_max_info() {
		ui_element_min_max_info ret;

		// Note that we play nicely with separate horizontal & vertical DPI.
		// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
		CSize DPI = QueryScreenDPIEx(*this);

		if (DPI.cx <= 0 || DPI.cy <= 0) { // sanity
			DPI = CSize(96, 96);
		}


		ret.m_min_width = MulDiv(400, DPI.cx, 96);
		ret.m_min_height = MulDiv(100, DPI.cy, 96);
		ret.m_max_width = MulDiv(400, DPI.cx, 96);
		ret.m_max_height = MulDiv(100, DPI.cy, 96);

		// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
		ret.adjustForWindow(*this);

		return ret;
	}

private:
	int RateInHertz() const
	{
		return (int)(round(params[kGargleRate] * 99.0f) + 1);
	}

	uint32_t shit;

	static uint32_t parseConfig(ui_element_config::ptr cfg) {
		return 1;

	}
	static ui_element_config::ptr makeConfig(bool init = false) {
		ui_element_config_builder out;

		if (init)
		{
			uint32_t crap = 1;
			out << crap;
		}
		else
		{
			uint32_t crap = 2;
			out << crap;
		}
		return out.finish(g_get_guid());
	}

	void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
	bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

	void EchoDisable() {
		static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_gargle);
	}

	void EchoEnable(float params[], bool echo_enabled) {
		dsp_preset_impl preset;
		dsp_dmo_gargle::make_preset(params, echo_enabled, preset);
		static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
	}

	void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		if (IsEchoEnabled()) {
			GetConfig();
			dsp_preset_impl preset;
			dsp_dmo_gargle::make_preset(params, echo_enabled, preset);
			//yes change api;
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}
		else {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_gargle);
		}

	}

	void ApplySettings()
	{
		dsp_preset_impl preset;
		if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_dmo_gargle, preset)) {
			SetEchoEnabled(true);
			dsp_dmo_gargle::parse_preset(params, echo_enabled, preset);
			SetEchoEnabled(echo_enabled);
			SetConfig();
		}
		else {
			SetEchoEnabled(false);
			SetConfig();
		}
	}


	void GetConfig()
	{
		params[kGargleRate] = slider_gain.GetPos() / 1000.0;
		int gain = RateInHertz();
		echo_enabled = IsEchoEnabled();

		RefreshLabel(gain);
	}

	void SetConfig()
	{
		slider_gain.SetPos((double)(1000 * params[kGargleRate]));
		int gain = RateInHertz();
		RefreshLabel(gain);
	}

	BOOL OnInitDialog(CWindow, LPARAM)
	{

		modeless_dialog_manager::g_add(m_hWnd);
		m_buttonEchoEnabled = GetDlgItem(IDC_GARGLEENABLED);
		m_buttonEchoEnabled.ShowWindow(SW_SHOW);

		slider_gain = GetDlgItem(IDC_GARGLEFREQ1);
		slider_gain.SetRange(0, 1000);
		float params2[kEqNumParameters] = { 0 };
		params2[kGargleRate] = 0.02f;
		params2[kGargleWaveShape] = 0.0f;
		pfc::copy_array_loop_t(params, params2, kEqNumParameters);
		ApplySettings();
		return TRUE;
	}

	void OnConfigChanged() {
		if (IsEchoEnabled()) {
			EchoEnable(params, echo_enabled);
		}
		else {
			EchoDisable();
		}

	}

	void OnButton(UINT, int id, CWindow)
	{
		DestroyWindow();
	}


	void OnScroll(UINT scrollID, int pos, CWindow window)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			if (LOWORD(scrollID) != SB_THUMBTRACK)
			{
				EchoEnable(params, echo_enabled);
			}
		}

	}

	void OnChange(UINT, int id, CWindow)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			OnConfigChanged();
		}
	}

	void RefreshLabel(int gain)
	{
		pfc::string_formatter msg;
		msg << "Rate: ";
		msg << pfc::format_int(gain) << " Hz";
		::uSetDlgItemText(*this, IDC_GARGLEFREQLAB1, msg);
		msg.reset();
	}
	bool m_ownEchoUpdate;
	float params[kEqNumParameters];
	CTrackBarCtrl slider_gain;
	CButton m_buttonEchoEnabled;
	bool echo_enabled;
protected:
	const ui_element_instance_callback::ptr m_callback;
};

class myElem_t5 : public  ui_element_impl_withpopup< uielem_gargle > {
	bool get_element_group(pfc::string_base& p_out)
	{
		p_out = "Effect DSP";
		return true;
	}

	bool get_menu_command_description(pfc::string_base& out) {
		out = "Opens a window for gargle effects.";
		return true;
	}

};
static service_factory_single_t<myElem_t5> g_myElemFactory5;







static void RunConfigPopupWaves(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback);
class dsp_dmo_waves : public dsp_impl_base
{
	enum Parameters
	{
		kRvbInGain = 0,
		kRvbReverbMix,
		kRvbReverbTime,
		kRvbHighFreqRTRatio,
		kDistNumParameters
	};
	float m_param[kDistNumParameters];
	int m_rate, m_ch, m_ch_mask;
	bool enabled;
	DMOFilter dmo_filt;
public:
	static GUID g_get_guid()
	{
		// {B904DAC3-5DAE-4E9B-96B5-3D92BE33BF38}
		return guid_dmo_waves;
	}

	dsp_dmo_waves(dsp_preset const& in) :m_rate(0), m_ch(0), m_ch_mask(0)
	{
		parse_preset(m_param, enabled, in);
		enabled = true;
	}
	~dsp_dmo_waves()
	{
		dmo_filt.deinit();
	}

	static void g_get_name(pfc::string_base& p_out) { p_out = "Waves Reverb (DirectX)"; }

	bool on_chunk(audio_chunk* chunk, abort_callback&)
	{
		if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
		{
			m_rate = chunk->get_srate();
			m_ch = chunk->get_channels();
			m_ch_mask = chunk->get_channel_config();
			dmo_filt.deinit();
			if (m_ch <= 2)
			{
				dmo_filt.init(m_rate, DMO_Type::REVERBWAVES, m_ch);
				for (int i = 0; i < kDistNumParameters; i++)
					dmo_filt.setparameter(i, m_param[i]);
			}

		}
		if (m_ch <= 2)
		{
			audio_sample* data = chunk->get_data();
			unsigned count = chunk->get_data_size();
			dmo_filt.Process(data, count);
		}


		return true;
	}

	void on_endofplayback(abort_callback&) { }
	void on_endoftrack(abort_callback&) { }

	void flush()
	{
		dmo_filt.deinit();
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

	static bool g_get_default_preset(dsp_preset & p_out)
	{
		float params[kDistNumParameters] =
		{
	  1.0f,
	1.0f,
	 1.0f / 3.0f,
     0.0f
		};
		make_preset(params, true, p_out);
		return true;
	}
	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		::RunConfigPopupWaves(p_data, p_parent, p_callback);
	}
	static bool g_have_config_popup() { return true; }

	static void make_preset(float params[kDistNumParameters], bool enabled, dsp_preset & out)
	{
		dsp_preset_builder builder;
		for (int i = 0; i < kDistNumParameters; i++)
			builder << params[i];
		builder << enabled;
		builder.finish(g_get_guid(), out);
	}

	static void parse_preset(float* params, bool& enabled, const dsp_preset & in)
	{
		try
		{
			dsp_preset_parser parser(in);
			for (int i = 0; i < kDistNumParameters; i++)
			{
				parser >> params[i];
			}
			parser >> enabled;
		}
		catch (exception_io_data) {

			params[kRvbInGain] = 1.0f;
			params[kRvbReverbMix] = 1.0f;
			params[kRvbReverbTime] = 1.0f / 3.0f;
			params[kRvbHighFreqRTRatio] = 0.0f;
			enabled = true;
		}
	}
};

static dsp_factory_t<dsp_dmo_waves> g_dsp_waves_factory;

class CMyDSPPopupWaves : public CDialogImpl<CMyDSPPopupWaves>
{
public:
	CMyDSPPopupWaves(const dsp_preset& initData, dsp_preset_edit_callback& callback) : m_initData(initData), m_callback(callback) { }
	enum { IDD = IDD_WAVREVERB };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kRvbInGain = 0,
		kRvbReverbMix,
		kRvbReverbTime,
		kRvbHighFreqRTRatio,
		kDistNumParameters
	};

	BEGIN_MSG_MAP(CMyDSPPopupWaves)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		MSG_WM_HSCROLL(OnHScroll)
	END_MSG_MAP()

private:

	static float GainInDecibel(float param) { return -96.0f + param * 96.0f; }
	float ReverbTime() const { return 0.001f + params[kRvbReverbTime] * 2999.999f; }
	float HighFreqRTRatio() const { return 0.001f + params[kRvbHighFreqRTRatio] * 0.998f; }

	BOOL OnInitDialog(CWindow, LPARAM)
	{
		slider_gain = GetDlgItem(IDC_WAVGAIN);
		slider_gain.SetRange(0, 100);
		slider_mix = GetDlgItem(IDC_WAVREVERBMIX);
		slider_mix.SetRange(0, 100);
		slider_time = GetDlgItem(IDC_WAVEREVERBTIME);
		slider_time.SetRange(0, 100);
		bool enabled;
		dsp_dmo_waves::parse_preset(params, enabled, m_initData);
		slider_gain.SetPos((double)(100 * params[kRvbInGain]));
		slider_mix.SetPos((double)(100 * params[kRvbReverbMix]));
		slider_time.SetPos((double)(100 * params[kRvbReverbTime]));

		float gain = GainInDecibel(params[kRvbInGain]);
		float mix = GainInDecibel(params[kRvbReverbMix]);
		float time = ReverbTime();

		RefreshLabel(gain,mix,time);
		return TRUE;
	}

	void OnButton(UINT, int id, CWindow)
	{
		EndDialog(id);
	}

	void OnChange(UINT, int id, CWindow)
	{
		params[kRvbInGain] = slider_gain.GetPos() / 100.0;
		params[kRvbReverbMix] = slider_mix.GetPos() / 100.0;
		params[kRvbReverbTime] = slider_time.GetPos() / 100.0;

		dsp_preset_impl preset;
		dsp_dmo_waves::make_preset(params, true, preset);
		m_callback.on_preset_changed(preset);
		int gain = GainInDecibel(params[kRvbInGain]);
		int mix = GainInDecibel(params[kRvbReverbMix]);
		float time = ReverbTime();
		RefreshLabel(gain, mix, time);
	}

	void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
	{
		params[kRvbInGain] = slider_gain.GetPos() / 100.0;
		params[kRvbReverbMix] = slider_mix.GetPos() / 100.0;
		params[kRvbReverbTime] = slider_time.GetPos() / 100.0;

		if (LOWORD(nSBCode) != SB_THUMBTRACK)
		{
			dsp_preset_impl preset;
			dsp_dmo_waves::make_preset(params, true, preset);
			m_callback.on_preset_changed(preset);
		}
		int gain = GainInDecibel(params[kRvbInGain]);
		int mix = GainInDecibel(params[kRvbReverbMix]);
		float time = ReverbTime();
		RefreshLabel(gain, mix, time);
	}

	void RefreshLabel(float gain,float mix,float time)
	{
		pfc::string_formatter msg;
		msg << "Gain: ";
		msg << pfc::format_int(gain) << " dB";
		::uSetDlgItemText(*this, IDC_WAVGAINLAB, msg);
		msg.reset();
		msg << "Mix: ";
		msg << pfc::format_int(mix) << " dB";
		::uSetDlgItemText(*this, IDC_WAVREVERBMIXLAB, msg);
		msg.reset();
		msg << "Time: ";
		msg << pfc::format_int(time) << " ms";
		::uSetDlgItemText(*this, IDC_WAVEREVERBTIMELAB, msg);
		msg.reset();
	}

	const dsp_preset& m_initData; // modal dialog so we can reference this caller-owned object.
	dsp_preset_edit_callback& m_callback;
	float params[kDistNumParameters];
	CTrackBarCtrl slider_gain,slider_mix,slider_time;
};

static void RunConfigPopupWaves(const dsp_preset& p_data, HWND p_parent, dsp_preset_edit_callback& p_callback)
{
	CMyDSPPopupWaves popup(p_data, p_callback);
	if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
}

// {D1A3B3F0-0FF4-44D6-B06B-44FBD679A3E3}
// {A31B034D-A673-475C-BF56-B2668C568613}
static const GUID guid_waveselem =
{ 0xa31b034d, 0xa673, 0x475c, { 0xbf, 0x56, 0xb2, 0x66, 0x8c, 0x56, 0x86, 0x13 } };





class uielem_waves : public CDialogImpl<uielem_waves>, public ui_element_instance {
public:
	uielem_waves(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
		echo_enabled = true;
	}
	enum { IDD = IDD_WAVREVERB1 };
	enum
	{
		FreqMin = 0,
		FreqMax = 2000,
		FreqRangeTotal = FreqMax - FreqMin,
		depthmin = 0,
		depthmax = 100,
	};

	enum Parameters
	{
		kRvbInGain = 0,
		kRvbReverbMix,
		kRvbReverbTime,
		kRvbHighFreqRTRatio,
		kDistNumParameters
	};

	BEGIN_MSG_MAP(uielem_waves)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
		COMMAND_HANDLER_EX(IDC_WAVENABLE, BN_CLICKED, OnEnabledToggle)
		MSG_WM_HSCROLL(OnScroll)
	END_MSG_MAP()

	void initialize_window(HWND parent) { WIN32_OP(Create(parent) != NULL); }
	HWND get_wnd() { return m_hWnd; }
	void set_configuration(ui_element_config::ptr config) {
		shit = parseConfig(config);
		if (m_hWnd != NULL) {
			ApplySettings();
		}
		m_callback->on_min_max_info_change();
	}
	ui_element_config::ptr get_configuration() { return makeConfig(); }
	static GUID g_get_guid() {
		return guid_waveselem;
	}
	static void g_get_name(pfc::string_base & out) { out = "Waves Reverb (DirectX)"; }
	static ui_element_config::ptr g_get_default_configuration() {
		return makeConfig(true);
	}
	static const char* g_get_description() { return "Modifies the 'Waves Reverb' DSP effect."; }
	static GUID g_get_subclass() {
		return ui_element_subclass_dsp;
	}

	ui_element_min_max_info get_min_max_info() {
		ui_element_min_max_info ret;

		// Note that we play nicely with separate horizontal & vertical DPI.
		// Such configurations have not been ever seen in circulation, but nothing stops us from supporting such.
		CSize DPI = QueryScreenDPIEx(*this);

		if (DPI.cx <= 0 || DPI.cy <= 0) { // sanity
			DPI = CSize(96, 96);
		}


		ret.m_min_width = MulDiv(320, DPI.cx, 96);
		ret.m_min_height = MulDiv(130, DPI.cy, 96);
		ret.m_max_width = MulDiv(320, DPI.cx, 96);
		ret.m_max_height = MulDiv(130, DPI.cy, 96);

		// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
		ret.adjustForWindow(*this);

		return ret;
	}

private:
	static float GainInDecibel(float param) { return -96.0f + param * 96.0f; }
	float ReverbTime() const { return 0.001f + params[kRvbReverbTime] * 2999.999f; }
	float HighFreqRTRatio() const { return 0.001f + params[kRvbHighFreqRTRatio] * 0.998f; }

	uint32_t shit;

	static uint32_t parseConfig(ui_element_config::ptr cfg) {
		return 1;

	}
	static ui_element_config::ptr makeConfig(bool init = false) {
		ui_element_config_builder out;

		if (init)
		{
			uint32_t crap = 1;
			out << crap;
		}
		else
		{
			uint32_t crap = 2;
			out << crap;
		}
		return out.finish(g_get_guid());
	}

	void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
	bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

	void EchoDisable() {
		static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_waves);
	}

	void EchoEnable(float params[], bool echo_enabled) {
		dsp_preset_impl preset;
		dsp_dmo_waves::make_preset(params, echo_enabled, preset);
		static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
	}

	void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		if (IsEchoEnabled()) {
			GetConfig();
			dsp_preset_impl preset;
			dsp_dmo_waves::make_preset(params, echo_enabled, preset);
			//yes change api;
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}
		else {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_dmo_waves);
		}

	}

	void ApplySettings()
	{
		dsp_preset_impl preset;
		if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_dmo_waves, preset)) {
			SetEchoEnabled(true);
			dsp_dmo_waves::parse_preset(params, echo_enabled, preset);
			SetEchoEnabled(echo_enabled);
			SetConfig();
		}
		else {
			SetEchoEnabled(false);
			SetConfig();
		}
	}


	void GetConfig()
	{
		params[kRvbInGain] = slider_gain.GetPos() / 100.0;
		params[kRvbReverbMix] = slider_mix.GetPos() / 100.0;
		params[kRvbReverbTime] = slider_time.GetPos() / 100.0;
		int gain = GainInDecibel(params[kRvbInGain]);
		int mix = GainInDecibel(params[kRvbReverbMix]);
		float time = ReverbTime();
		echo_enabled = IsEchoEnabled();
		RefreshLabel(gain, mix, time);
	}

	void SetConfig()
	{
		slider_gain.SetPos((double)(100 * params[kRvbInGain]));
		slider_mix.SetPos((double)(100 * params[kRvbReverbMix]));
		slider_time.SetPos((double)(100 * params[kRvbReverbTime]));

		float gain = GainInDecibel(params[kRvbInGain]);
		float mix = GainInDecibel(params[kRvbReverbMix]);
		float time = ReverbTime();

		RefreshLabel(gain, mix, time);
	}

	BOOL OnInitDialog(CWindow, LPARAM)
	{

		modeless_dialog_manager::g_add(m_hWnd);
		m_buttonEchoEnabled = GetDlgItem(IDC_WAVENABLE);
		m_buttonEchoEnabled.ShowWindow(SW_SHOW);

		slider_gain = GetDlgItem(IDC_WAVGAIN);
		slider_gain.SetRange(0, 100);
		slider_mix = GetDlgItem(IDC_WAVREVERBMIX);
		slider_mix.SetRange(0, 100);
		slider_time = GetDlgItem(IDC_WAVEREVERBTIME);
		slider_time.SetRange(0, 100);
		float params2[kDistNumParameters] = { 0 };
		params2[kRvbInGain] = 1.0f;
		params2[kRvbReverbMix] = 1.0f;
		params2[kRvbReverbTime] = 1.0f / 3.0f;
		params2[kRvbHighFreqRTRatio] = 0.0f;
		pfc::copy_array_loop_t(params, params2, kDistNumParameters);
		ApplySettings();
		return TRUE;
	}

	void OnConfigChanged() {
		if (IsEchoEnabled()) {
			EchoEnable(params, echo_enabled);
		}
		else {
			EchoDisable();
		}

	}

	void OnButton(UINT, int id, CWindow)
	{
		DestroyWindow();
	}


	void OnScroll(UINT scrollID, int pos, CWindow window)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			if (LOWORD(scrollID) != SB_THUMBTRACK)
			{
				EchoEnable(params, echo_enabled);
			}
		}

	}

	void OnChange(UINT, int id, CWindow)
	{
		pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
		GetConfig();
		if (IsEchoEnabled())
		{
			OnConfigChanged();
		}
	}

	void RefreshLabel(float gain, float mix, float time)
	{
		pfc::string_formatter msg;
		msg << "Gain: ";
		msg << pfc::format_int(gain) << " dB";
		::uSetDlgItemText(*this, IDC_WAVGAINLAB, msg);
		msg.reset();
		msg << "Mix: ";
		msg << pfc::format_int(mix) << " dB";
		::uSetDlgItemText(*this, IDC_WAVREVERBMIXLAB, msg);
		msg.reset();
		msg << "Time: ";
		msg << pfc::format_int(time) << " ms";
		::uSetDlgItemText(*this, IDC_WAVEREVERBTIMELAB, msg);
		msg.reset();
	}
	bool m_ownEchoUpdate;
	float params[kDistNumParameters];
	CTrackBarCtrl slider_gain,slider_mix,slider_time;
	CButton m_buttonEchoEnabled;
	bool echo_enabled;
protected:
	const ui_element_instance_callback::ptr m_callback;
};

class myElem_t7 : public  ui_element_impl_withpopup< uielem_waves > {
	bool get_element_group(pfc::string_base& p_out)
	{
		p_out = "Effect DSP";
		return true;
	}

	bool get_menu_command_description(pfc::string_base& out) {
		out = "Opens a window for reverberation effects.";
		return true;
	}

};
static service_factory_single_t<myElem_t7> g_myElemFactory7;


