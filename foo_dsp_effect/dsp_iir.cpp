#include "../helpers/foobar2000+atl.h"
#include "../../libPPUI/win32_utility.h"
#include "../../libPPUI/win32_op.h" // WIN32_OP()
#include "../helpers/BumpableElem.h"
#include "resource.h"
#include "iirfilters.h"
#include "dsp_guids.h"

namespace {
	class CEditMod : public CWindowImpl<CEditMod, CEdit >
	{
	public:
		BEGIN_MSG_MAP(CEditMod)
			MESSAGE_HANDLER(WM_CHAR, OnChar)
		END_MSG_MAP()

		CEditMod(HWND hWnd = NULL) { }
		LRESULT OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			switch (wParam)
			{
			case '\r': //Carriage return
				::PostMessage(m_parent, WM_USER, 0x1988, 0L);
				return 0;
				break;
			}
			return DefWindowProc(uMsg, wParam, lParam);
		}
		void AttachToDlgItem(HWND parent)
		{
			m_parent = parent;
		}
	private:
		UINT m_dlgItem;
		HWND m_parent;
	};

	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);

	class dsp_iir : public dsp_impl_base
	{
		int m_rate, m_ch, m_ch_mask;
		int p_freq; //40.0, 13000.0 (Frequency: Hz)
		int p_gain; //gain
		int p_type; //filter type
		float p_qual;
		bool iir_enabled;
		pfc::array_t<IIRFilter> m_buffers;
	public:
		static GUID g_get_guid()
		{
			// {FEA092A6-EA54-4f62-B180-4C88B9EB2B67}

			return guid_iir;
		}

		dsp_iir(dsp_preset const & in) :m_rate(0), m_ch(0), m_ch_mask(0), p_freq(400),p_qual(0.707), p_gain(10), p_type(0)
		{
			iir_enabled = true;
			parse_preset(p_freq, p_gain, p_type,p_qual, iir_enabled, in);
		}

		static void g_get_name(pfc::string_base & p_out) { p_out = "IIR Filter"; }

		bool on_chunk(audio_chunk * chunk, abort_callback &)
		{
			if (!iir_enabled)return true;
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
					e.setQuality(p_qual);
					e.setGain(p_gain);
					e.init(m_rate, p_type);
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
		static bool g_get_default_preset(dsp_preset & p_out)
		{
			make_preset(400, 10, 0,0.707, true, p_out);
			return true;
		}
		static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
		{
			::RunConfigPopup(p_data, p_parent, p_callback);
		}
		static bool g_have_config_popup() { return true; }

		static void make_preset(int p_freq, int p_gain, int p_type,float p_quality, bool enabled, dsp_preset & out)
		{
			dsp_preset_builder builder;
			builder << p_freq;
			builder << p_gain; //gain
			builder << p_type; //filter type
			builder << p_quality;
			builder << enabled;
			builder.finish(g_get_guid(), out);
		}
		static void parse_preset(int & p_freq, int & p_gain, int & p_type,float & p_quality, bool & enabled, const dsp_preset & in)
		{
			try
			{
				dsp_preset_parser parser(in);
				parser >> p_freq;
				parser >> p_gain; //gain
				parser >> p_type; //filter type
				parser >> p_quality;
				parser >> enabled;
			}
			catch (exception_io_data) { p_freq = 400; p_gain = 10; p_type = 0; p_quality = 0.707; enabled = true; }
		}
	};

	class CMyDSPPopupIIR : public CDialogImpl<CMyDSPPopupIIR>
	{
	public:
		CMyDSPPopupIIR(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }
		enum { IDD = IDD_IIR1 };

		enum
		{
			FreqMin = 0,
			FreqMax = 40000,
			FreqRangeTotal = FreqMax,
			GainMin = -100,
			GainMax = 100,
			GainRangeTotal = GainMax - GainMin
		};

		BEGIN_MSG_MAP(CMyDSPPopup)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
			COMMAND_HANDLER_EX(IDC_IIRTYPE, CBN_SELCHANGE, OnChange)
			MSG_WM_HSCROLL(OnScroll)
			MESSAGE_HANDLER(WM_USER, OnEditControlChange)
		END_MSG_MAP()

	private:
		LRESULT OnEditControlChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			if (wParam == 0x1988)
			{
				GetEditText();
			}
			return 0;
		}

		void GetEditText()
		{
			CString sWindowText;
			pitch_edit.GetWindowText(sWindowText);
			float pitch2 = _ttof(sWindowText);
			if (pitch_s != sWindowText)
			{
			
				dsp_preset_impl preset;
				dsp_iir::make_preset(p_freq, p_gain, p_type, pitch2, true, preset);
				p_qual = pitch2;
				m_callback.on_preset_changed(preset);
			}
		}

		void DSPConfigChange(dsp_chain_config const & cfg)
		{
			if (m_hWnd != NULL) {
				ApplySettings();
			}
		}

		void ApplySettings()
		{
			dsp_preset_impl preset2;
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_iir, preset2)) {
				bool enabled;
				dsp_iir::parse_preset(p_freq, p_gain, p_type,p_qual, enabled, preset2);
				slider_freq.SetPos(p_freq);
				slider_gain.SetPos(p_gain);
				CWindow w = GetDlgItem(IDC_IIRTYPE1);
				::SendMessage(w, CB_SETCURSEL, p_type, 0);
				if (p_type == 10)
				{
					slider_freq.EnableWindow(FALSE);
					slider_gain.EnableWindow(FALSE);
				}
				else
				{
					slider_freq.EnableWindow(TRUE);
					slider_gain.EnableWindow(TRUE);
				}
				RefreshLabel(p_freq, p_gain, p_type);
			}
		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{

			pitch_edit.AttachToDlgItem(m_hWnd);
			pitch_edit.SubclassWindow(GetDlgItem(IDC_IIRQ1));
			slider_freq = GetDlgItem(IDC_IIRFREQ);
			slider_freq.SetRangeMin(0);
			slider_freq.SetRangeMax(FreqMax);
			slider_gain = GetDlgItem(IDC_IIRGAIN);
			slider_gain.SetRange(GainMin, GainMax);
			{

				bool enabled;
				dsp_iir::parse_preset(p_freq, p_gain, p_type,p_qual, enabled, m_initData);
				if (p_type == 10)
				{
					slider_freq.EnableWindow(FALSE);
					slider_gain.EnableWindow(FALSE);
				}
				else
				{
					slider_freq.EnableWindow(TRUE);
					slider_gain.EnableWindow(TRUE);
				}



				slider_freq.SetPos(p_freq);
				slider_gain.SetPos(p_gain);
				CWindow w = GetDlgItem(IDC_IIRTYPE);
				uSendMessageText(w, CB_ADDSTRING, 0, "Resonant Lowpass");
				uSendMessageText(w, CB_ADDSTRING, 0, "Resonant Highpass");
				uSendMessageText(w, CB_ADDSTRING, 0, "Bandpass (CSG)");
				uSendMessageText(w, CB_ADDSTRING, 0, "Bandpass (ZPG)");
				uSendMessageText(w, CB_ADDSTRING, 0, "Allpass");
				uSendMessageText(w, CB_ADDSTRING, 0, "Notch");
				uSendMessageText(w, CB_ADDSTRING, 0, "RIAA Tape/Vinyl De-emphasis");
				uSendMessageText(w, CB_ADDSTRING, 0, "Parametric EQ (single band)");
				uSendMessageText(w, CB_ADDSTRING, 0, "Bass Boost");
				uSendMessageText(w, CB_ADDSTRING, 0, "Low shelf");
				uSendMessageText(w, CB_ADDSTRING, 0, "CD De-emphasis");
				uSendMessageText(w, CB_ADDSTRING, 0, "High shelf");
				::SendMessage(w, CB_SETCURSEL, p_type, 0);
				RefreshLabel(p_freq, p_gain, p_type);

			}
			return TRUE;
		}

		void OnButton(UINT, int id, CWindow)
		{
			GetEditText();
			EndDialog(id);
		}

		void OnChange(UINT scrollid, int id, CWindow window)
		{
			CWindow w;
			p_freq = slider_freq.GetPos();
			p_gain = slider_gain.GetPos();
			p_type = SendDlgItemMessage(IDC_IIRTYPE, CB_GETCURSEL);
			{
				dsp_preset_impl preset;
				dsp_iir::make_preset(p_freq, p_gain, p_type,p_qual, true, preset);
				m_callback.on_preset_changed(preset);
			}
			if (p_type == 10) {
				slider_freq.EnableWindow(FALSE);
				slider_gain.EnableWindow(FALSE);
			}
			else
			{
				slider_freq.EnableWindow(TRUE);
				slider_gain.EnableWindow(TRUE);
			}
			RefreshLabel(p_freq, p_gain, p_type);

		}
		void OnScroll(UINT scrollid, int id, CWindow window)
		{
			CWindow w;
			p_freq = slider_freq.GetPos();
			p_gain = slider_gain.GetPos();
			p_type = SendDlgItemMessage(IDC_IIRTYPE, CB_GETCURSEL);
			if (LOWORD(scrollid) != SB_THUMBTRACK)
			{
				dsp_preset_impl preset;
				dsp_iir::make_preset(p_freq, p_gain, p_type,p_qual, true, preset);
				m_callback.on_preset_changed(preset);
			}
			if (p_type == 10) {
				slider_freq.EnableWindow(FALSE);
				slider_gain.EnableWindow(FALSE);
			}
			else
			{
				slider_freq.EnableWindow(TRUE);
				slider_gain.EnableWindow(TRUE);
			}
			RefreshLabel(p_freq, p_gain, p_type);

		}


		void RefreshLabel(int p_freq, int p_gain, int p_type)
		{
			pfc::string_formatter msg;

			if (p_type == 10)
			{
				msg << "Frequency: disabled";
				::uSetDlgItemText(*this, IDC_IIRFREQINFO, msg);
				msg.reset();
				msg << "Gain: disabled";
				::uSetDlgItemText(*this, IDC_IIRGAININFO, msg);
				return;

			}
			msg << "Frequency: ";
			msg << pfc::format_int(p_freq) << " Hz";
			::uSetDlgItemText(*this, IDC_IIRFREQINFO, msg);
			msg.reset();
			msg << "Gain: ";
			msg << pfc::format_int(p_gain) << " db";
			::uSetDlgItemText(*this, IDC_IIRGAININFO, msg);

			msg.reset();
			msg << pfc::format_float(p_qual, 0, 3);
			CString sWindowText;
			sWindowText = msg.c_str();
			pitch_s = sWindowText;
			pitch_edit.SetWindowText(sWindowText);
		}
		int p_freq;
		int p_gain;
		int p_type;
		float p_qual;
		CEditMod pitch_edit;
		CString pitch_s;
		const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
		dsp_preset_edit_callback & m_callback;
		CTrackBarCtrl slider_freq, slider_gain;
	};

	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		CMyDSPPopupIIR popup(p_data, p_callback);
		if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
	}


	static dsp_factory_t<dsp_iir> g_dsp_iir_factory;







	// {1DC17CA0-0023-4266-AD59-691D566AC291}
	static const GUID guid_choruselem =
	{ 0xf875c614, 0x439f, 0x4c53,{ 0xb2, 0xef, 0xa6, 0x6e, 0x17, 0x4b, 0xf0, 0x23 } };


	class uielem_iir : public CDialogImpl<uielem_iir>, public ui_element_instance {
	public:
		uielem_iir(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
			p_freq = 400; p_gain = 10; p_type = 0;
			p_qual = 0.707;
			IIR_enabled = true;

		}
		enum { IDD = IDD_IIR };
		enum
		{
			FreqMin = 0,
			FreqMax = 40000,
			FreqRangeTotal = FreqMax,
			GainMin = -100,
			GainMax = 100,
			GainRangeTotal = GainMax - GainMin
		};
		BEGIN_MSG_MAP(uielem_iir)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDC_IIRENABLED, BN_CLICKED, OnEnabledToggle)
			MSG_WM_HSCROLL(OnScroll)
			COMMAND_HANDLER_EX(IDC_IIRTYPE1, CBN_SELCHANGE, OnChange1)
			MESSAGE_HANDLER(WM_USER, OnEditControlChange)
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
		static void g_get_name(pfc::string_base & out) { out = "IIR Filter"; }
		static ui_element_config::ptr g_get_default_configuration() {
			return makeConfig(true);
		}
		static const char * g_get_description() { return "Modifies the 'IIR Filter' DSP effect."; }
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
			ret.m_min_height = MulDiv(140, DPI.cy, 96);
			ret.m_max_width = MulDiv(420, DPI.cx, 96);
			ret.m_max_height = MulDiv(140, DPI.cy, 96);

			// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
			ret.adjustForWindow(*this);

			return ret;
		}

	private:
		LRESULT OnEditControlChange(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
		{
			if (wParam == 0x1988)
			{
				GetEditText();
			}
			return 0;
		}

		void OnChange1(UINT scrollid, int id, CWindow window)
		{
			CWindow w;
			p_freq = slider_freq.GetPos();
			p_gain = slider_gain.GetPos();
			p_type = SendDlgItemMessage(IDC_IIRTYPE1, CB_GETCURSEL);
			if(IsIIREnabled())
			{
				dsp_preset_impl preset;
				dsp_iir::make_preset(p_freq, p_gain, p_type, p_qual, true, preset);
				static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);

			}
			if (p_type == 10) {
				slider_freq.EnableWindow(FALSE);
				slider_gain.EnableWindow(FALSE);
			}
			else
			{
				slider_freq.EnableWindow(TRUE);
				slider_gain.EnableWindow(TRUE);
			}
			RefreshLabel(p_freq, p_gain, p_type);

		}

		void GetEditText()
		{
			CString sWindowText;
			pitch_edit.GetWindowText(sWindowText);
			float pitch2 = _ttof(sWindowText);
			if (pitch_s != sWindowText)
			{
				if (IsIIREnabled())
				{
					dsp_preset_impl preset;
					dsp_iir::make_preset(p_freq, p_gain, p_type, pitch2, true, preset);
					static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
					p_qual = pitch2;
				}
				
			}
		}


		void SetIIREnabled(bool state) { m_buttonIIREnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
		bool IsIIREnabled() { return m_buttonIIREnabled == NULL || m_buttonIIREnabled.GetCheck() == BST_CHECKED; }

		void IIRDisable() {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_iir);
		}


		void IIREnable(int p_freq, int p_gain, int p_type,float p_qual, bool IIR_enabled) {
			dsp_preset_impl preset;
			dsp_iir::make_preset(p_freq, p_gain, p_type, p_qual, IIR_enabled, preset);
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}

		void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
			pfc::vartoggle_t<bool> ownUpdate(m_ownIIRUpdate, true);
			if (IsIIREnabled()) {
				GetConfig();
				dsp_preset_impl preset;
				dsp_iir::make_preset(p_freq, p_gain, p_type, p_qual, IIR_enabled, preset);
				//yes change api;
				static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
			}
			else {
				static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_iir);
			}

		}

		void OnScroll(UINT scrollID, int pos, CWindow window)
		{
			pfc::vartoggle_t<bool> ownUpdate(m_ownIIRUpdate, true);
			GetConfig();
			if (IsIIREnabled())
			{
				if (LOWORD(scrollID) != SB_THUMBTRACK)
				{
					IIREnable(p_freq, p_gain, p_type,p_qual, IIR_enabled);
				}
			}

		}

		void OnChange(UINT, int id, CWindow)
		{
			pfc::vartoggle_t<bool> ownUpdate(m_ownIIRUpdate, true);
			GetConfig();
			if (IsIIREnabled())
			{

				OnConfigChanged();
			}
		}

		void DSPConfigChange(dsp_chain_config const & cfg)
		{
			if (!m_ownIIRUpdate && m_hWnd != NULL) {
				ApplySettings();
			}
		}

		//set settings if from another control
		void ApplySettings()
		{
			dsp_preset_impl preset;
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_iir, preset)) {
				SetIIREnabled(true);
				dsp_iir::parse_preset(p_freq, p_gain, p_type,p_qual, IIR_enabled, preset);
				SetIIREnabled(IIR_enabled);
				SetConfig();
			}
			else {
				SetIIREnabled(false);
				SetConfig();
			}
		}

		void OnConfigChanged() {
			if (IsIIREnabled()) {
				IIREnable(p_freq, p_gain, p_type, p_qual, IIR_enabled);
			}
			else {
				IIRDisable();
			}

		}


		void GetConfig()
		{
			p_freq = slider_freq.GetPos();
			p_gain = slider_gain.GetPos();
			p_type = SendDlgItemMessage(IDC_IIRTYPE1, CB_GETCURSEL);
			IIR_enabled = IsIIREnabled();
			RefreshLabel(p_freq, p_gain, p_type);


		}

		void SetConfig()
		{
			slider_freq.SetPos(p_freq);
			slider_gain.SetPos(p_gain);
			CWindow w = GetDlgItem(IDC_IIRTYPE1);
			::SendMessage(w, CB_SETCURSEL, p_type, 0);
			RefreshLabel(p_freq, p_gain, p_type);

		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			pitch_edit.AttachToDlgItem(m_hWnd);
			pitch_edit.SubclassWindow(GetDlgItem(IDC_IIRQ));
			slider_freq = GetDlgItem(IDC_IIRFREQ1);
			slider_freq.SetRangeMin(0);
			slider_freq.SetRangeMax(FreqMax);
			slider_gain = GetDlgItem(IDC_IIRGAIN1);
			slider_gain.SetRange(GainMin, GainMax);
			CWindow w = GetDlgItem(IDC_IIRTYPE1);
			uSendMessageText(w, CB_ADDSTRING, 0, "Resonant Lowpass");
			uSendMessageText(w, CB_ADDSTRING, 0, "Resonant Highpass");
			uSendMessageText(w, CB_ADDSTRING, 0, "Bandpass (CSG)");
			uSendMessageText(w, CB_ADDSTRING, 0, "Bandpass (ZPG)");
			uSendMessageText(w, CB_ADDSTRING, 0, "Allpass");
			uSendMessageText(w, CB_ADDSTRING, 0, "Notch");
			uSendMessageText(w, CB_ADDSTRING, 0, "RIAA Tape/Vinyl De-emphasis");
			uSendMessageText(w, CB_ADDSTRING, 0, "Parametric EQ (single band)");
			uSendMessageText(w, CB_ADDSTRING, 0, "Bass Boost");
			uSendMessageText(w, CB_ADDSTRING, 0, "Low shelf");
			uSendMessageText(w, CB_ADDSTRING, 0, "CD De-emphasis");
			uSendMessageText(w, CB_ADDSTRING, 0, "High shelf");


			m_buttonIIREnabled = GetDlgItem(IDC_IIRENABLED);
			m_ownIIRUpdate = false;

			ApplySettings();
			return TRUE;
		}

		void RefreshLabel(int p_freq, int p_gain, int p_type)
		{
			pfc::string_formatter msg;

			if (p_type == 10)
			{
				msg << "Frequency: disabled";
				::uSetDlgItemText(*this, IDC_IIRFREQINFO1, msg);
				msg.reset();
				msg << "Gain: disabled";
				::uSetDlgItemText(*this, IDC_IIRGAININFO1, msg);
				return;

			}
			msg << "Frequency: ";
			msg << pfc::format_int(p_freq) << " Hz";
			::uSetDlgItemText(*this, IDC_IIRFREQINFO1, msg);
			msg.reset();
			msg << "Gain: ";
			msg << pfc::format_int(p_gain) << " db";
			::uSetDlgItemText(*this, IDC_IIRGAININFO1, msg);

			msg.reset();
			msg << pfc::format_float(p_qual, 0, 3);
			CString sWindowText;
			sWindowText = msg.c_str();
			pitch_s = sWindowText;
			pitch_edit.SetWindowText(sWindowText);
		}

		bool IIR_enabled;
		int p_freq; //40.0, 13000.0 (Frequency: Hz)
		int p_gain; //gain
		int p_type; //filter type
		float p_qual;
		CEditMod pitch_edit;
		CString pitch_s;
		CTrackBarCtrl slider_freq, slider_gain;
		CButton m_buttonIIREnabled;
		bool m_ownIIRUpdate;

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

	class myElem_t : public  ui_element_impl_withpopup< uielem_iir > {
		bool get_element_group(pfc::string_base & p_out)
		{
			p_out = "Effect DSP";
			return true;
		}

		bool get_menu_command_description(pfc::string_base & out) {
			out = "Opens a window for IIR filtering control.";
			return true;
		}

	};
	static service_factory_single_t<myElem_t> g_myElemFactory;


}
