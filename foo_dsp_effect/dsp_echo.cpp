#define MYVERSION "0.6"
#include "../helpers/foobar2000+atl.h"
#include "../helpers/BumpableElem.h"
#include "resource.h"
#include "echo.h"
#include "dsp_guids.h"

namespace {

	static void RunDSPConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);

	class dsp_echo : public dsp_impl_base
	{
		int m_rate, m_ch, m_ch_mask;
		int m_ms, m_amp, m_fb;
		bool enabled;
		pfc::array_t<Echo> m_buffers;
	public:
		dsp_echo(dsp_preset const & in) : m_rate(0), m_ch(0), m_ch_mask(0), m_ms(200), m_amp(128), m_fb(128)
		{
			enabled = true;
			parse_preset(m_ms, m_amp, m_fb, enabled, in);
		}

		static GUID g_get_guid()
		{

			return guid_echo;
		}

		static void g_get_name(pfc::string_base & p_out) { p_out = "Echo"; }

		bool on_chunk(audio_chunk * chunk, abort_callback &)
		{
			if (!enabled)return true;
			if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
			{
				m_rate = chunk->get_srate();
				m_ch = chunk->get_channels();
				m_ch_mask = chunk->get_channel_config();
				m_buffers.set_count(0);
				m_buffers.set_count(m_ch);
				for (unsigned i = 0; i < m_ch; i++)
				{
					Echo & e = m_buffers[i];
					e.SetSampleRate(m_rate);
					e.SetDelay(m_ms);
					e.SetAmp(m_amp);
					e.SetFeedback(m_fb);
				}
			}

			for (unsigned i = 0; i < m_ch; i++)
			{
				Echo & e = m_buffers[i];
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
		static bool g_get_default_preset(dsp_preset & p_out)
		{
			make_preset(200, 128, 128, true, p_out);
			return true;
		}
		static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
		{
			::RunDSPConfigPopup(p_data, p_parent, p_callback);
		}
		static bool g_have_config_popup() { return true; }
		static void make_preset(int ms, int amp, int feedback, bool enabled, dsp_preset & out)
		{
			dsp_preset_builder builder; builder << ms; builder << amp; builder << feedback; builder << enabled; builder.finish(g_get_guid(), out);
		}
		static void parse_preset(int & ms, int & amp, int & feedback, bool enabled, const dsp_preset & in)
		{
			try
			{
				dsp_preset_parser parser(in); parser >> ms; parser >> amp; parser >> feedback; parser >> enabled;
			}
			catch (exception_io_data) { ms = 200; amp = 128; feedback = 128; enabled = true; }
		}
	};

	static dsp_factory_t<dsp_echo> g_dsp_echo_factory;

	// {1DC17CA0-0023-4266-AD59-691D566AC291}
	static const GUID guid_choruselem
	{ 0x634933f1, 0xbdb6, 0x4d4c,{ 0x8a, 0x68, 0x28, 0xb9, 0xd, 0xc0, 0xfe, 0x3 } };

	class uielem_echo : public CDialogImpl<uielem_echo>, public ui_element_instance {
	public:
		uielem_echo(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
			ms = 200;
			amp = 128;
			feedback = 128;
			echo_enabled = true;

		}
		enum { IDD = IDD_ECHO1 };
		enum
		{
			MSRangeMin = 10,
			MSRangeMax = 5000,

			MSRangeTotal = MSRangeMax - MSRangeMin,

			AmpRangeMin = 0,
			AmpRangeMax = 256,

			AmpRangeTotal = AmpRangeMax - AmpRangeMin
		};
		BEGIN_MSG_MAP(uielem_echo)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDC_ECHOENABLED, BN_CLICKED, OnEnabledToggle)
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
			return guid_choruselem;
		}
		static void g_get_name(pfc::string_base & out) { out = "Echo"; }
		static ui_element_config::ptr g_get_default_configuration() {
			return makeConfig(true);
		}
		static const char * g_get_description() { return "Modifies the 'Echo' DSP effect."; }
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


			ret.m_min_width = MulDiv(420, DPI.cx, 96);
			ret.m_min_height = MulDiv(240, DPI.cy, 96);
			ret.m_max_width = MulDiv(420, DPI.cx, 96);
			ret.m_max_height = MulDiv(240, DPI.cy, 96);

			// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
			ret.adjustForWindow(*this);

			return ret;
		}

	private:
		void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
		bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

		void EchoDisable() {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_echo);
		}


		void EchoEnable(int ms, int amp, int feedback, bool echo_enabled) {
			dsp_preset_impl preset;
			dsp_echo::make_preset(ms, amp, feedback, echo_enabled, preset);
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}

		void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
			pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
			if (IsEchoEnabled()) {
				GetConfig();
				dsp_preset_impl preset;
				dsp_echo::make_preset(ms, amp, feedback, echo_enabled, preset);
				//yes change api;
				static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
			}
			else {
				static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_echo);
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
					EchoEnable(ms, amp, feedback, echo_enabled);
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

		void DSPConfigChange(dsp_chain_config const & cfg)
		{
			if (!m_ownEchoUpdate && m_hWnd != NULL) {
				ApplySettings();
			}
		}

		//set settings if from another control
		void ApplySettings()
		{
			dsp_preset_impl preset;
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_echo, preset)) {
				SetEchoEnabled(true);
				dsp_echo::parse_preset(ms, amp, feedback, echo_enabled, preset);
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
				EchoEnable(ms, amp, feedback, echo_enabled);
			}
			else {
				EchoDisable();
			}

		}


		void GetConfig()
		{
			ms = m_slider_ms.GetPos() + MSRangeMin;
			amp = m_slider_amp.GetPos() + AmpRangeMin;
			feedback = m_slider_fb.GetPos() + AmpRangeMin;
			echo_enabled = IsEchoEnabled();
			RefreshLabel(ms, amp, feedback);


		}

		void SetConfig()
		{
			m_slider_ms.SetPos(pfc::clip_t<t_int32>(ms, MSRangeMin, MSRangeMax) - MSRangeMin);
			m_slider_amp.SetPos(pfc::clip_t<t_int32>(amp, AmpRangeMin, AmpRangeMax) - AmpRangeMin);
			m_slider_fb.SetPos(pfc::clip_t<t_int32>(feedback, AmpRangeMin, AmpRangeMax) - AmpRangeMin);

			RefreshLabel(ms, amp, feedback);

		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{

			m_slider_ms = GetDlgItem(IDC_SLIDER_MS1);
			m_slider_ms.SetRange(0, MSRangeTotal);

			m_slider_amp = GetDlgItem(IDC_SLIDER_AMP1);
			m_slider_amp.SetRange(0, AmpRangeTotal);

			m_slider_fb = GetDlgItem(IDC_SLIDER_FB1);
			m_slider_fb.SetRange(0, AmpRangeTotal);


			m_buttonEchoEnabled = GetDlgItem(IDC_ECHOENABLED);
			m_ownEchoUpdate = false;

			ApplySettings();
			return TRUE;
		}

		void RefreshLabel(int ms, int amp, int feedback)
		{
			pfc::string_formatter msg; msg << "Delay time: " << pfc::format_int(ms) << " ms";
			::uSetDlgItemText(*this, IDC_SLIDER_LABEL_MS1, msg);
			msg.reset(); msg << "Echo volume: " << pfc::format_int(amp * 100 / 256) << "%";
			::uSetDlgItemText(*this, IDC_SLIDER_LABEL_AMP1, msg);
			msg.reset(); msg << "Echo feedback: " << pfc::format_int(feedback * 100 / 256) << "%";
			::uSetDlgItemText(*this, IDC_SLIDER_LABEL_FB1, msg);
		}

		bool echo_enabled;
		int ms, amp, feedback;
		CTrackBarCtrl m_slider_ms, m_slider_amp, m_slider_fb;
		CButton m_buttonEchoEnabled;
		bool m_ownEchoUpdate;

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
		uint32_t shit;
	protected:
		const ui_element_instance_callback::ptr m_callback;
	};

	class myElem_t : public  ui_element_impl_withpopup< uielem_echo > {
		bool get_element_group(pfc::string_base & p_out)
		{
			p_out = "Effect DSP";
			return true;
		}

		bool get_menu_command_description(pfc::string_base & out) {
			out = "Opens a window for echo effects.";
			return true;
		}

	};
	static service_factory_single_t<myElem_t> g_myElemFactory;


	class CMyDSPPopup : public CDialogImpl<CMyDSPPopup>
	{
	public:
		CMyDSPPopup(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }

		enum { IDD = IDD_ECHO };

		enum
		{
			MSRangeMin = 10,
			MSRangeMax = 5000,

			MSRangeTotal = MSRangeMax - MSRangeMin,

			AmpRangeMin = 0,
			AmpRangeMax = 256,

			AmpRangeTotal = AmpRangeMax - AmpRangeMin
		};

		BEGIN_MSG_MAP(CMyDSPPopup)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
			MSG_WM_HSCROLL(OnHScroll)
		END_MSG_MAP()

	private:
		void DSPConfigChange(dsp_chain_config const & cfg)
		{
			if (m_hWnd != NULL) {
				ApplySettings();
			}
		}

		void ApplySettings()
		{
			dsp_preset_impl preset2;
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_echo, preset2)) {
				bool enabled;
				bool dynamics_enabled;
				dsp_echo::parse_preset(ms, amp, feedback, enabled, preset2);
				m_slider_ms.SetPos(pfc::clip_t<t_int32>(ms, MSRangeMin, MSRangeMax) - MSRangeMin);
				m_slider_amp.SetPos(pfc::clip_t<t_int32>(amp, AmpRangeMin, AmpRangeMax) - AmpRangeMin);
				m_slider_fb.SetPos(pfc::clip_t<t_int32>(amp, AmpRangeMin, AmpRangeMax) - AmpRangeMin);
				RefreshLabel(ms, amp, feedback);
			}
		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			m_slider_ms = GetDlgItem(IDC_SLIDER_MS);
			m_slider_ms.SetRange(0, MSRangeTotal);

			m_slider_amp = GetDlgItem(IDC_SLIDER_AMP);
			m_slider_amp.SetRange(0, AmpRangeTotal);
			m_slider_fb = GetDlgItem(IDC_SLIDER_FB);
			m_slider_fb.SetRange(0, AmpRangeTotal);

			{

				bool enabled = true;
				dsp_echo::parse_preset(ms, amp, feedback, enabled, m_initData);
				m_slider_ms.SetPos(pfc::clip_t<t_int32>(ms, MSRangeMin, MSRangeMax) - MSRangeMin);
				m_slider_amp.SetPos(pfc::clip_t<t_int32>(amp, AmpRangeMin, AmpRangeMax) - AmpRangeMin);
				m_slider_fb.SetPos(pfc::clip_t<t_int32>(feedback, AmpRangeMin, AmpRangeMax) - AmpRangeMin);
				RefreshLabel(ms, amp, feedback);
			}
			return TRUE;
		}

		void OnButton(UINT, int id, CWindow)
		{
			EndDialog(id);
		}

		void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
		{
			ms = m_slider_ms.GetPos() + MSRangeMin;
			amp = m_slider_amp.GetPos() + AmpRangeMin;
			feedback = m_slider_fb.GetPos() + AmpRangeMin;
			if (LOWORD(nSBCode) != SB_THUMBTRACK)
			{
				dsp_preset_impl preset;
				dsp_echo::make_preset(ms, amp, feedback, true, preset);
				m_callback.on_preset_changed(preset);
			}
			RefreshLabel(ms, amp, feedback);
		}

		void RefreshLabel(int ms, int amp, int feedback)
		{
			pfc::string_formatter msg; msg << "Delay time: " << pfc::format_int(ms) << " ms";
			::uSetDlgItemText(*this, IDC_SLIDER_LABEL_MS, msg);
			msg.reset(); msg << "Echo volume: " << pfc::format_int(amp * 100 / 256) << "%";
			::uSetDlgItemText(*this, IDC_SLIDER_LABEL_AMP, msg);
			msg.reset(); msg << "Echo feedback: " << pfc::format_int(feedback * 100 / 256) << "%";
			::uSetDlgItemText(*this, IDC_SLIDER_LABEL_FB, msg);
		}

		const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
		dsp_preset_edit_callback & m_callback;
		int ms, amp, feedback;
		CTrackBarCtrl m_slider_ms, m_slider_amp, m_slider_fb;
	};

	static void RunDSPConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		CMyDSPPopup popup(p_data, p_callback);
		if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
	}

}
