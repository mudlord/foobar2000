#include "../helpers/foobar2000+atl.h"
#include "../../libPPUI/win32_utility.h"
#include "../../libPPUI/win32_op.h" // WIN32_OP()
#include "../helpers/BumpableElem.h"
#include "resource.h"
#include "wahwah.h"
#include "dsp_guids.h"

namespace {
	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);
	class dsp_wahwah : public dsp_impl_base
	{
		int m_rate, m_ch, m_ch_mask;
		float freq;
		float depth, startphase;
		float freqofs, res;
		bool enabled;
		pfc::array_t<WahWah> m_buffers;
	public:
		static GUID g_get_guid()
		{
			// {3E144CFA-C63A-4c12-9503-A5C83C7E5CF8}
			return guid_wahwah;
		}

		dsp_wahwah(dsp_preset const & in) : m_rate(0), m_ch(0), m_ch_mask(0), freq(1.5), depth(0.70), startphase(0.0), freqofs(0.30), res(2.5)
		{
			enabled = true;
			parse_preset(freq, depth, startphase, freqofs, res, enabled, in);
		}

		static void g_get_name(pfc::string_base & p_out) { p_out = "WahWah"; }

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
					WahWah & e = m_buffers[i];
					e.SetDepth(depth);
					e.SetFreqOffset(freqofs);
					e.SetLFOFreq(freq);
					e.SetLFOStartPhase(startphase);
					e.SetResonance(res);
					e.init(m_rate);
				}
			}

			for (unsigned i = 0; i < m_ch; i++)
			{
				WahWah & e = m_buffers[i];
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
			make_preset(1.5, 0.70, 0.0, 0.30, 2.5, true, p_out);
			return true;
		}
		static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
		{
			::RunConfigPopup(p_data, p_parent, p_callback);
		}
		static bool g_have_config_popup() { return true; }


		static void make_preset(float freq, float depth, float startphase, float freqofs, float res, bool enabled, dsp_preset & out)
		{
			dsp_preset_builder builder;
			builder << freq;
			builder << depth;
			builder << startphase;
			builder << freqofs;
			builder << res;
			builder << enabled;
			builder.finish(g_get_guid(), out);
		}
		static void parse_preset(float & freq, float & depth, float & startphase, float & freqofs, float & res, bool & enabled, const dsp_preset & in)
		{
			try
			{
				dsp_preset_parser parser(in);
				parser >> freq;
				parser >> depth;
				parser >> startphase;
				parser >> freqofs;
				parser >> res;
				parser >> enabled;
			}
			catch (exception_io_data) { freq = 1.5; depth = 0.70; startphase = 0.0; freqofs = 0.3; res = 2.5; enabled = true; }
		}
	};


	// {1DC17CA0-0023-4266-AD59-691D566AC291}
	static const GUID guid_choruselem =
	{ 0xf7d39b6, 0x43f, 0x4e5c,{ 0x8d, 0x5a, 0xa5, 0x3f, 0x42, 0xb6, 0xf, 0xcc } };



	class uielem_wah : public CDialogImpl<uielem_wah>, public ui_element_instance {
	public:
		uielem_wah(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
			freq = 1.5; depth = 0.70; startphase = 0.0; freqofs = 0.3; res = 2.5; wah_enabled = true;

		}
		enum { IDD = IDD_WAHWAH1 };

		enum
		{
			FreqMin = 1,
			FreqMax = 200,
			FreqRangeTotal = FreqMax - FreqMin,
			StartPhaseMin = 0,
			StartPhaseMax = 360,
			StartPhaseTotal = 360,
			DepthMin = 0,
			DepthMax = 100,
			DepthRangeTotal = 100,
			ResonanceMin = 0,
			ResonanceMax = 100,
			ResonanceTotal = 100,
			FrequencyOffsetMin = 0,
			FrequencyOffsetMax = 100,
			FrequencyOffsetTotal = 100
		};
		BEGIN_MSG_MAP(uielem_wah)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDC_WAHENABLED, BN_CLICKED, OnEnabledToggle)
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
		static void g_get_name(pfc::string_base & out) { out = "WahWah"; }
		static ui_element_config::ptr g_get_default_configuration() {
			return makeConfig(true);
		}
		static const char * g_get_description() { return "Modifies the 'WahWah' DSP effect."; }
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


			ret.m_min_width = MulDiv(380, DPI.cx, 96);
			ret.m_min_height = MulDiv(260, DPI.cy, 96);
			ret.m_max_width = MulDiv(380, DPI.cx, 96);
			ret.m_max_height = MulDiv(260, DPI.cy, 96);

			// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
			ret.adjustForWindow(*this);

			return ret;
		}

	private:
		void SetWahEnabled(bool state) { m_buttonWahEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
		bool IsWahEnabled() { return m_buttonWahEnabled == NULL || m_buttonWahEnabled.GetCheck() == BST_CHECKED; }

		void WahDisable() {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_wahwah);
		}


		void WahEnable(float freq, float depth, float startphase, float freqofs, float res, bool wah_enabled) {
			dsp_preset_impl preset;
			dsp_wahwah::make_preset(freq, depth, startphase, freqofs, res, wah_enabled, preset);
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}

		void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
			pfc::vartoggle_t<bool> ownUpdate(m_ownWahUpdate, true);
			if (IsWahEnabled()) {
				GetConfig();
				dsp_preset_impl preset;
				dsp_wahwah::make_preset(freq, depth, startphase, freqofs, res, wah_enabled, preset);
				//yes change api;
				static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
			}
			else {
				static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_wahwah);
			}

		}

		void OnScroll(UINT scrollID, int pos, CWindow window)
		{
			pfc::vartoggle_t<bool> ownUpdate(m_ownWahUpdate, true);
			GetConfig();
			if (IsWahEnabled())
			{
				if (LOWORD(scrollID) != SB_THUMBTRACK)
				{
					WahEnable(freq, depth, startphase, freqofs, res, wah_enabled);
				}
			}

		}

		void OnChange(UINT, int id, CWindow)
		{
			pfc::vartoggle_t<bool> ownUpdate(m_ownWahUpdate, true);
			GetConfig();
			if (IsWahEnabled())
			{
				OnConfigChanged();
			}
		}

		void DSPConfigChange(dsp_chain_config const & cfg)
		{
			if (!m_ownWahUpdate && m_hWnd != NULL) {
				ApplySettings();
			}
		}

		//set settings if from another control
		void ApplySettings()
		{
			dsp_preset_impl preset;
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_wahwah, preset)) {
				SetWahEnabled(true);
				dsp_wahwah::parse_preset(freq, depth, startphase, freqofs, res, wah_enabled, preset);
				SetWahEnabled(wah_enabled);
				SetConfig();
			}
			else {
				SetWahEnabled(false);
				SetConfig();
			}
		}

		void OnConfigChanged() {
			if (IsWahEnabled()) {
				WahEnable(freq, depth, startphase, freqofs, res, wah_enabled);
			}
			else {
				WahDisable();
			}

		}


		void GetConfig()
		{
			freq = slider_freq.GetPos() / 10.0;
			depth = slider_depth.GetPos() / 100.0;
			startphase = slider_startphase.GetPos() / 100.0;
			freqofs = slider_freqofs.GetPos() / 100.0;
			res = slider_res.GetPos() / 10.0;
			wah_enabled = IsWahEnabled();
			RefreshLabel(freq, depth, startphase, freqofs, res);
		}

		void SetConfig()
		{
			slider_freq.SetPos((double)(10 * freq));
			slider_startphase.SetPos((double)(100 * startphase));
			slider_freqofs.SetPos((double)(100 * freqofs));
			slider_depth.SetPos((double)(100 * depth));
			slider_res.SetPos((double)(10 * res));

			RefreshLabel(freq, depth, startphase, freqofs, res);

		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			slider_freq = GetDlgItem(IDC_WAHLFOFREQ1);
			slider_freq.SetRange(FreqMin, FreqMax);
			slider_startphase = GetDlgItem(IDC_WAHLFOSTARTPHASE1);
			slider_startphase.SetRange(0, StartPhaseMax);
			slider_freqofs = GetDlgItem(IDC_WAHFREQOFFSET1);
			slider_freqofs.SetRange(0, 100);
			slider_depth = GetDlgItem(IDC_WAHDEPTH1);
			slider_depth.SetRange(0, 100);
			slider_res = GetDlgItem(IDC_WAHRESONANCE1);
			slider_res.SetRange(0, 100);

			m_buttonWahEnabled = GetDlgItem(IDC_WAHENABLED);
			m_ownWahUpdate = false;

			ApplySettings();
			return TRUE;
		}

		void RefreshLabel(float freq, float depth, float startphase, float freqofs, float res)
		{
			pfc::string_formatter msg;
			msg << "LFO Frequency: ";
			msg << pfc::format_float(freq, 0, 1) << " Hz";
			::uSetDlgItemText(*this, IDC_WAHLFOFREQINFO1, msg);
			msg.reset();
			msg << "Depth: ";
			msg << pfc::format_int(depth * 100) << "%";
			::uSetDlgItemText(*this, IDC_WAHDEPTHINFO1, msg);
			msg.reset();
			msg << "LFO Starting Phase: ";
			msg << pfc::format_int(startphase * 100) << " (deg.)";
			::uSetDlgItemText(*this, IDC_WAHLFOPHASEINFO1, msg);
			msg.reset();
			msg << "Wah Frequency Offset: ";
			msg << pfc::format_int(freqofs * 100) << "%";
			::uSetDlgItemText(*this, IDC_WAHFREQOFFSETINFO1, msg);
			msg.reset();
			msg << "Resonance: ";
			msg << pfc::format_float(res, 0, 1) << "";
			::uSetDlgItemText(*this, IDC_WAHRESONANCEINFO1, msg);
			msg.reset();
		}

		bool wah_enabled;
		float freq;
		float depth, startphase;
		float freqofs, res;
		CTrackBarCtrl slider_freq, slider_startphase, slider_freqofs, slider_depth, slider_res;
		CButton m_buttonWahEnabled;
		bool m_ownWahUpdate;

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

	class myElem_t : public  ui_element_impl_withpopup< uielem_wah > {
		bool get_element_group(pfc::string_base & p_out)
		{
			p_out = "Effect DSP";
			return true;
		}

		bool get_menu_command_description(pfc::string_base & out) {
			out = "Opens a window for wah control.";
			return true;
		}

	};
	static service_factory_single_t<myElem_t> g_myElemFactory;

	class CMyDSPPopupWah : public CDialogImpl<CMyDSPPopupWah>
	{
	public:
		CMyDSPPopupWah(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }
		enum { IDD = IDD_WAHWAH };

		enum
		{
			FreqMin = 1,
			FreqMax = 200,
			FreqRangeTotal = FreqMax - FreqMin,
			StartPhaseMin = 0,
			StartPhaseMax = 360,
			StartPhaseTotal = 360,
			DepthMin = 0,
			DepthMax = 100,
			DepthRangeTotal = 100,
			ResonanceMin = 0,
			ResonanceMax = 100,
			ResonanceTotal = 100,
			FrequencyOffsetMin = 0,
			FrequencyOffsetMax = 100,
			FrequencyOffsetTotal = 100
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
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_wahwah, preset2)) {
				bool enabled;
				dsp_wahwah::parse_preset(freq, depth, startphase, freqofs, res, enabled, m_initData);

				slider_freq.SetPos((double)(10 * freq));
				slider_startphase.SetPos((double)(100 * startphase));
				slider_freqofs.SetPos((double)(100 * freqofs));
				slider_depth.SetPos((double)(100 * depth));
				slider_res.SetPos((double)(10 * res));

				RefreshLabel(freq, depth, startphase, freqofs, res);
			}
		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			slider_freq = GetDlgItem(IDC_WAHLFOFREQ);
			slider_freq.SetRange(FreqMin, FreqMax);
			slider_startphase = GetDlgItem(IDC_WAHLFOSTARTPHASE);
			slider_startphase.SetRange(0, StartPhaseMax);
			slider_freqofs = GetDlgItem(IDC_WAHFREQOFFSET);
			slider_freqofs.SetRange(0, 100);
			slider_depth = GetDlgItem(IDC_WAHDEPTH);
			slider_depth.SetRange(0, 100);
			slider_res = GetDlgItem(IDC_WAHRESONANCE);
			slider_res.SetRange(0, 100);

			{
				bool enabled;
				dsp_wahwah::parse_preset(freq, depth, startphase, freqofs, res, enabled, m_initData);

				slider_freq.SetPos((double)(10 * freq));
				slider_startphase.SetPos((double)(100 * startphase));
				slider_freqofs.SetPos((double)(100 * freqofs));
				slider_depth.SetPos((double)(100 * depth));
				slider_res.SetPos((double)(10 * res));

				RefreshLabel(freq, depth, startphase, freqofs, res);


			}
			return TRUE;
		}

		void OnButton(UINT, int id, CWindow)
		{
			EndDialog(id);
		}

		void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
		{
			freq = slider_freq.GetPos() / 10.0;
			depth = slider_depth.GetPos() / 100.0;
			startphase = slider_startphase.GetPos() / 100.0;
			freqofs = slider_freqofs.GetPos() / 100.0;
			res = slider_res.GetPos() / 10.0;
			if (LOWORD(nSBCode) != SB_THUMBTRACK)
			{
				dsp_preset_impl preset;
				dsp_wahwah::make_preset(freq, depth, startphase, freqofs, res, true, preset);
				m_callback.on_preset_changed(preset);
			}
			RefreshLabel(freq, depth, startphase, freqofs, res);

		}

		void RefreshLabel(float freq, float depth, float startphase, float freqofs, float res)
		{
			pfc::string_formatter msg;
			msg << "LFO Frequency: ";
			msg << pfc::format_float(freq, 0, 1) << " Hz";
			::uSetDlgItemText(*this, IDC_WAHLFOFREQINFO, msg);
			msg.reset();
			msg << "Depth: ";
			msg << pfc::format_int(depth * 100) << "%";
			::uSetDlgItemText(*this, IDC_WAHDEPTHINFO, msg);
			msg.reset();
			msg << "LFO Starting Phase: ";
			msg << pfc::format_int(startphase * 100) << " (deg.)";
			::uSetDlgItemText(*this, IDC_WAHLFOPHASEINFO, msg);
			msg.reset();
			msg << "Wah Frequency Offset: ";
			msg << pfc::format_int(freqofs * 100) << "%";
			::uSetDlgItemText(*this, IDC_WAHFREQOFFSETINFO, msg);
			msg.reset();
			msg << "Resonance: ";
			msg << pfc::format_float(res, 0, 1) << "";
			::uSetDlgItemText(*this, IDC_WAHRESONANCEINFO, msg);
			msg.reset();
		}

		const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
		dsp_preset_edit_callback & m_callback;
		float freq;
		float depth, startphase;
		float freqofs, res;
		CTrackBarCtrl slider_freq, slider_startphase, slider_freqofs, slider_depth, slider_res;
	};

	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		CMyDSPPopupWah popup(p_data, p_callback);
		if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
	}

	static dsp_factory_t<dsp_wahwah> g_dsp_wahwah_factory;

}
