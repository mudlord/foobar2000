#include <math.h>
#include "../helpers/foobar2000+atl.h"
#include "../../libPPUI/win32_utility.h"
#include "../../libPPUI/win32_op.h" // WIN32_OP()
#include "../helpers/BumpableElem.h"
#include "resource.h"
#include "freeverb.h"
#include "dsp_guids.h"

namespace {
	static void RunDSPConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);
	class dsp_freeverb : public dsp_impl_base
	{
		int m_rate, m_ch, m_ch_mask;
		float drytime;
		float wettime;
		float dampness;
		float roomwidth;
		float roomsize;
		bool enabled;
		pfc::array_t<revmodel> m_buffers;
	public:

		dsp_freeverb(dsp_preset const & in) : drytime(0.43), wettime(0.57), dampness(0.45), roomwidth(0.56), roomsize(0.56), m_rate(0), m_ch(0), m_ch_mask(0)
		{
			enabled = true;
			parse_preset(drytime, wettime, dampness, roomwidth, roomsize, enabled, in);
		}
		static GUID g_get_guid()
		{
			// {97C60D5F-3572-4d35-9260-FD0CF5DBA480}

			return guid_freeverb;
		}
		static void g_get_name(pfc::string_base & p_out) { p_out = "Reverb"; }
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
					revmodel & e = m_buffers[i];
					e.init(m_rate);
					e.setwet(wettime);
					e.setdry(drytime);
					e.setdamp(dampness);
					e.setroomsize(roomsize);
					e.setwidth(roomwidth);
				}
			}
			for (unsigned i = 0; i < m_ch; i++)
			{
				revmodel & e = m_buffers[i];
				audio_sample * data = chunk->get_data() + i;
				for (unsigned j = 0, k = chunk->get_sample_count(); j < k; j++)
				{
					*data = e.processsample(*data);
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
			make_preset(0.43, 0.57, 0.45, 0.56, 0.56, true, p_out);
			return true;
		}
		static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
		{
			::RunDSPConfigPopup(p_data, p_parent, p_callback);
		}
		static bool g_have_config_popup() { return true; }
		static void make_preset(float drytime, float wettime, float dampness, float roomwidth, float roomsize, bool enabled, dsp_preset & out)
		{
			dsp_preset_builder builder;
			builder << drytime;
			builder << wettime;
			builder << dampness;
			builder << roomwidth;
			builder << roomsize;
			builder << enabled;
			builder.finish(g_get_guid(), out);
		}
		static void parse_preset(float & drytime, float & wettime, float & dampness, float & roomwidth, float & roomsize, bool & enabled, const dsp_preset & in)
		{
			try
			{
				dsp_preset_parser parser(in);
				parser >> drytime;
				parser >> wettime;
				parser >> dampness;
				parser >> roomwidth;
				parser >> roomsize;
				parser >> enabled;
			}
			catch (exception_io_data) { drytime = 0.43; wettime = 0.57; dampness = 0.45; roomwidth = 0.56; roomsize = 0.56; enabled = true; }
		}
	};

	// {1DC17CA0-0023-4266-AD59-691D566AC291}
	static const GUID guid_choruselem =
	{ 0x9afc1e0, 0xe9bb, 0x487b,{ 0x9b, 0xd8, 0x11, 0x3f, 0x29, 0x48, 0x8a, 0x90 } };



	class uielem_freeverb : public CDialogImpl<uielem_freeverb>, public ui_element_instance {
	public:
		uielem_freeverb(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
			drytime = 0.43; wettime = 0.57; dampness = 0.45;
			roomwidth = 0.56; roomsize = 0.56; reverb_enabled = true;

		}
		enum { IDD = IDD_REVERB1 };
		enum
		{
			drytimemin = 0,
			drytimemax = 100,
			drytimetotal = 100,
			wettimemin = 0,
			wettimemax = 100,
			wettimetotal = 100,
			dampnessmin = 0,
			dampnessmax = 100,
			dampnesstotal = 100,
			roomwidthmin = 0,
			roomwidthmax = 100,
			roomwidthtotal = 100,
			roomsizemin = 0,
			roomsizemax = 100,
			roomsizetotal = 100
		};

		BEGIN_MSG_MAP(uielem_freeverb)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDC_FREEVERBENABLE, BN_CLICKED, OnEnabledToggle)
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
		static void g_get_name(pfc::string_base & out) { out = "Reverb"; }
		static ui_element_config::ptr g_get_default_configuration() {
			return makeConfig(true);
		}
		static const char * g_get_description() { return "Modifies the reverberation DSP effect."; }
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


			ret.m_min_width = MulDiv(350, DPI.cx, 96);
			ret.m_min_height = MulDiv(240, DPI.cy, 96);
			ret.m_max_width = MulDiv(350, DPI.cx, 96);
			ret.m_max_height = MulDiv(240, DPI.cy, 96);

			// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
			ret.adjustForWindow(*this);

			return ret;
		}

	private:
		void SetReverbEnabled(bool state) { m_buttonReverbEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
		bool IsReverbEnabled() { return m_buttonReverbEnabled == NULL || m_buttonReverbEnabled.GetCheck() == BST_CHECKED; }

		void ReverbDisable() {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_freeverb);
		}


		void ReverbEnable(float  drytime, float wettime, float dampness, float roomwidth, float roomsize, bool reverb_enabled) {
			dsp_preset_impl preset;
			dsp_freeverb::make_preset(drytime, wettime, dampness, roomwidth, roomsize, reverb_enabled, preset);
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}

		void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
			pfc::vartoggle_t<bool> ownUpdate(m_ownReverbUpdate, true);
			if (IsReverbEnabled()) {
				GetConfig();
				dsp_preset_impl preset;
				dsp_freeverb::make_preset(drytime, wettime, dampness, roomwidth, roomsize, reverb_enabled, preset);
				//yes change api;
				static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
			}
			else {
				static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_freeverb);
			}

		}

		void OnScroll(UINT scrollID, int pos, CWindow window)
		{
			pfc::vartoggle_t<bool> ownUpdate(m_ownReverbUpdate, true);
			GetConfig();
			if (IsReverbEnabled())
			{
				if (LOWORD(scrollID) != SB_THUMBTRACK)
				{
					ReverbEnable(drytime, wettime, dampness, roomwidth, roomsize, reverb_enabled);
				}
			}

		}

		void OnChange(UINT, int id, CWindow)
		{
			pfc::vartoggle_t<bool> ownUpdate(m_ownReverbUpdate, true);
			GetConfig();
			if (IsReverbEnabled())
			{

				OnConfigChanged();
			}
		}

		void DSPConfigChange(dsp_chain_config const & cfg)
		{
			if (!m_ownReverbUpdate && m_hWnd != NULL) {
				ApplySettings();
			}
		}

		//set settings if from another control
		void ApplySettings()
		{
			dsp_preset_impl preset;
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_freeverb, preset)) {
				SetReverbEnabled(true);
				dsp_freeverb::parse_preset(drytime, wettime, dampness, roomwidth, roomsize, reverb_enabled, preset);
				SetReverbEnabled(reverb_enabled);
				SetConfig();
			}
			else {
				SetReverbEnabled(false);
				SetConfig();
			}
		}

		void OnConfigChanged() {
			if (IsReverbEnabled()) {
				ReverbEnable(drytime, wettime, dampness, roomwidth, roomsize, reverb_enabled);
			}
			else {
				ReverbDisable();
			}

		}


		void GetConfig()
		{
			drytime = slider_drytime.GetPos() / 100.0;
			wettime = slider_wettime.GetPos() / 100.0;
			dampness = slider_dampness.GetPos() / 100.0;
			roomwidth = slider_roomwidth.GetPos() / 100.0;
			roomsize = slider_roomsize.GetPos() / 100.0;
			reverb_enabled = IsReverbEnabled();
			RefreshLabel(drytime, wettime, dampness, roomwidth, roomsize);
		}

		void SetConfig()
		{
			slider_drytime.SetPos((double)(100 * drytime));
			slider_wettime.SetPos((double)(100 * wettime));
			slider_dampness.SetPos((double)(100 * dampness));
			slider_roomwidth.SetPos((double)(100 * roomwidth));
			slider_roomsize.SetPos((double)(100 * roomsize));
			RefreshLabel(drytime, wettime, dampness, roomwidth, roomsize);

		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			slider_drytime = GetDlgItem(IDC_DRYTIME1);
			slider_drytime.SetRange(0, drytimetotal);
			slider_wettime = GetDlgItem(IDC_WETTIME1);
			slider_wettime.SetRange(0, wettimetotal);
			slider_dampness = GetDlgItem(IDC_DAMPING1);
			slider_dampness.SetRange(0, dampnesstotal);
			slider_roomwidth = GetDlgItem(IDC_ROOMWIDTH1);
			slider_roomwidth.SetRange(0, roomwidthtotal);
			slider_roomsize = GetDlgItem(IDC_ROOMSIZE1);
			slider_roomsize.SetRange(0, roomsizetotal);

			m_buttonReverbEnabled = GetDlgItem(IDC_FREEVERBENABLE);
			m_ownReverbUpdate = false;

			ApplySettings();
			return TRUE;
		}

		void RefreshLabel(float  drytime, float wettime, float dampness, float roomwidth, float roomsize)
		{
			pfc::string_formatter msg;
			msg << "Dry Time: ";
			msg << pfc::format_int(100 * drytime) << "%";
			::uSetDlgItemText(*this, IDC_DRYTIMEINFO1, msg);
			msg.reset();
			msg << "Wet Time: ";
			msg << pfc::format_int(100 * wettime) << "%";
			::uSetDlgItemText(*this, IDC_WETTIMEINFO1, msg);
			msg.reset();
			msg << "Damping: ";
			msg << pfc::format_int(100 * dampness) << "%";
			::uSetDlgItemText(*this, IDC_DAMPINGINFO1, msg);
			msg.reset();
			msg << "Room Width: ";
			msg << pfc::format_int(100 * roomwidth) << "%";
			::uSetDlgItemText(*this, IDC_ROOMWIDTHINFO1, msg);
			msg.reset();
			msg << "Room Size: ";
			msg << pfc::format_int(100 * roomsize) << "%";
			::uSetDlgItemText(*this, IDC_ROOMSIZEINFO1, msg);
		}

		bool reverb_enabled;
		float  drytime, wettime, dampness, roomwidth, roomsize;
		CTrackBarCtrl slider_drytime, slider_wettime, slider_dampness, slider_roomwidth, slider_roomsize;
		CButton m_buttonReverbEnabled;
		bool m_ownReverbUpdate;

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

	class myElem_t : public  ui_element_impl_withpopup< uielem_freeverb > {
		bool get_element_group(pfc::string_base & p_out)
		{
			p_out = "Effect DSP";
			return true;
		}

		bool get_menu_command_description(pfc::string_base & out) {
			out = "Opens a window for reverberation effects control.";
			return true;
		}

	};
	static service_factory_single_t<myElem_t> g_myElemFactory;

	class CMyDSPPopupReverb : public CDialogImpl<CMyDSPPopupReverb>
	{
	public:
		CMyDSPPopupReverb(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }
		enum { IDD = IDD_REVERB };
		enum
		{
			drytimemin = 0,
			drytimemax = 100,
			drytimetotal = 100,
			wettimemin = 0,
			wettimemax = 100,
			wettimetotal = 100,
			dampnessmin = 0,
			dampnessmax = 100,
			dampnesstotal = 100,
			roomwidthmin = 0,
			roomwidthmax = 100,
			roomwidthtotal = 100,
			roomsizemin = 0,
			roomsizemax = 100,
			roomsizetotal = 100
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
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_freeverb, preset2)) {
				bool enabled;
				dsp_freeverb::parse_preset(drytime, wettime, dampness, roomwidth, roomsize, enabled, m_initData);

				slider_drytime.SetPos((double)(100 * drytime));
				slider_wettime.SetPos((double)(100 * wettime));
				slider_dampness.SetPos((double)(100 * dampness));
				slider_roomwidth.SetPos((double)(100 * roomwidth));
				slider_roomsize.SetPos((double)(100 * roomsize));

				RefreshLabel(drytime, wettime, dampness, roomwidth, roomsize);
			}
		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			slider_drytime = GetDlgItem(IDC_DRYTIME);
			slider_drytime.SetRange(0, drytimetotal);
			slider_wettime = GetDlgItem(IDC_WETTIME);
			slider_wettime.SetRange(0, wettimetotal);
			slider_dampness = GetDlgItem(IDC_DAMPING);
			slider_dampness.SetRange(0, dampnesstotal);
			slider_roomwidth = GetDlgItem(IDC_ROOMWIDTH);
			slider_roomwidth.SetRange(0, roomwidthtotal);
			slider_roomsize = GetDlgItem(IDC_ROOMSIZE);
			slider_roomsize.SetRange(0, roomsizetotal);

			{
				bool enabled;
				dsp_freeverb::parse_preset(drytime, wettime, dampness, roomwidth, roomsize, enabled, m_initData);

				slider_drytime.SetPos((double)(100 * drytime));
				slider_wettime.SetPos((double)(100 * wettime));
				slider_dampness.SetPos((double)(100 * dampness));
				slider_roomwidth.SetPos((double)(100 * roomwidth));
				slider_roomsize.SetPos((double)(100 * roomsize));

				RefreshLabel(drytime, wettime, dampness, roomwidth, roomsize);

			}
			return TRUE;
		}

		void OnButton(UINT, int id, CWindow)
		{
			EndDialog(id);
		}

		void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
		{
			drytime = slider_drytime.GetPos() / 100.0;
			wettime = slider_wettime.GetPos() / 100.0;
			dampness = slider_dampness.GetPos() / 100.0;
			roomwidth = slider_roomwidth.GetPos() / 100.0;
			roomsize = slider_roomsize.GetPos() / 100.0;
			if (LOWORD(nSBCode) != SB_THUMBTRACK)
			{
				dsp_preset_impl preset;
				dsp_freeverb::make_preset(drytime, wettime, dampness, roomwidth, roomsize, true, preset);
				m_callback.on_preset_changed(preset);
			}
			RefreshLabel(drytime, wettime, dampness, roomwidth, roomsize);
		}

		void RefreshLabel(float  drytime, float wettime, float dampness, float roomwidth, float roomsize)
		{
			pfc::string_formatter msg;
			msg << "Dry Time: ";
			msg << pfc::format_int(100 * drytime) << "%";
			::uSetDlgItemText(*this, IDC_DRYTIMEINFO, msg);
			msg.reset();
			msg << "Wet Time: ";
			msg << pfc::format_int(100 * wettime) << "%";
			::uSetDlgItemText(*this, IDC_WETTIMEINFO, msg);
			msg.reset();
			msg << "Damping: ";
			msg << pfc::format_int(100 * dampness) << "%";
			::uSetDlgItemText(*this, IDC_DAMPINGINFO, msg);
			msg.reset();
			msg << "Room Width: ";
			msg << pfc::format_int(100 * roomwidth) << "%";
			::uSetDlgItemText(*this, IDC_ROOMWIDTHINFO, msg);
			msg.reset();
			msg << "Room Size: ";
			msg << pfc::format_int(100 * roomsize) << "%";
			::uSetDlgItemText(*this, IDC_ROOMSIZEINFO, msg);
		}
		const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
		dsp_preset_edit_callback & m_callback;
		float  drytime, wettime, dampness, roomwidth, roomsize;
		CTrackBarCtrl slider_drytime, slider_wettime, slider_dampness, slider_roomwidth, slider_roomsize;
	};
	static void RunDSPConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		CMyDSPPopupReverb popup(p_data, p_callback);
		if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
	}

	static dsp_factory_t<dsp_freeverb> g_dsp_reverb_factory;

}
