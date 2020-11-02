
#define _USE_MATH_DEFINES
#include "../helpers/foobar2000+atl.h"
#include "../../libPPUI/win32_utility.h"
#include "../../libPPUI/win32_op.h" // WIN32_OP()
#include "../helpers/BumpableElem.h"
#include "resource.h"
#include "dsp_guids.h"

#include <vector>
namespace {

	const float BASE_DELAY_SEC = 0.002; // 2 ms
	const float VIBRATO_FREQUENCY_DEFAULT_HZ = 2;
	const float VIBRATO_FREQUENCY_MAX_HZ = 14;
	const float VIBRATO_DEPTH_DEFAULT_PERCENT = 50;

	__forceinline float getSampleHermite4p3o(float x, float *y)
	{
		static float c0, c1, c2, c3;
		// 4-point, 3rd-order Hermite (x-form)
		c0 = y[1];
		c1 = (1.0 / 2.0)*(y[2] - y[0]);
		c2 = (y[0] - (5.0 / 2.0)*y[1]) + (2.0*y[2] - (1.0 / 2.0)*y[3]);
		c3 = (1.0 / 2.0)*(y[3] - y[0]) + (3.0 / 2.0)*(y[1] - y[2]);
		return ((c3*x + c2)*x + c1)*x + c0;
	}

	class Vibrato
	{
	private:
		float freq;
		float samplerate;
		int phase;
		float depth;
		static const int additionalDelay = 3;
		float*  buffer;
		float *table;
		int writeIndex;
		int size;
	public:
		Vibrato()
		{
			buffer = NULL;
		}
		~Vibrato()
		{
			delete[]buffer;
			buffer = NULL;
		}
		void init(float freq, float depth, int samplerate)
		{
			size = BASE_DELAY_SEC * samplerate * 2;
			buffer = new float[size + additionalDelay];
			memset(buffer, 0, (size + additionalDelay) * sizeof(float));
			this->samplerate = samplerate;
			this->freq = freq;
			this->depth = depth;
			phase = 0;
			writeIndex = 0;

		}
		float Process(float in)
		{
			float M = freq / samplerate;
			int maxphase = samplerate / freq;
			float lfoValue = sin(M * 2. * M_PI * phase++);
			phase = phase % maxphase;
			lfoValue = (lfoValue + 1) * 1.; // transform from [-1; 1] to [0; 1]
			int maxDelay = BASE_DELAY_SEC * samplerate;
			float delay = lfoValue * depth * maxDelay;
			delay += additionalDelay;
			float fReadIndex = writeIndex - 1 - delay;
			while (fReadIndex < 0)fReadIndex += size;
			while (fReadIndex >= size)fReadIndex -= size;
			int iPart = (int)fReadIndex; // integer part of the delay
			float fPart = fReadIndex - iPart; // fractional part of the delay
			float value = getSampleHermite4p3o(fPart, &(buffer[iPart]));
			buffer[writeIndex] = in;
			if (writeIndex < additionalDelay) {
				buffer[size + writeIndex] = in;
			}
			writeIndex++;
			if (writeIndex == size) {
				writeIndex = 0;
			}
			return value;
		}
	};
	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);
	class dsp_vibrato : public dsp_impl_base
	{
		int m_rate, m_ch, m_ch_mask;
		float freq, depth;
		bool enabled;
		pfc::array_t<Vibrato> m_buffers;
	public:
		dsp_vibrato(dsp_preset const & in) :m_rate(0), m_ch(0), m_ch_mask(0) {
			// Mark buffer as empty.
			freq = 5.0;
			depth = 0.5;
			enabled = true;
			parse_preset(freq, depth, enabled, in);
		}

		// Every DSP type is identified by a GUID.
		static GUID g_get_guid() {
			return guid_vibrato;
		}

		// We also need a name, so the user can identify the DSP.
		// The name we use here does not describe what the DSP does,
		// so it would be a bad name. We can excuse this, because it
		// doesn't do anything useful anyway.
		static void g_get_name(pfc::string_base & p_out) {
			p_out = "Vibrato";
		}

		virtual void on_endoftrack(abort_callback & p_abort) {
			// This method is called when a track ends.
			// We need to do the same thing as flush(), so we just call it.
		}

		virtual void on_endofplayback(abort_callback & p_abort) {
			// This method is called on end of playback instead of flush().
			// We need to do the same thing as flush(), so we just call it.
		}

		// The framework feeds input to our DSP using this method.
		// Each chunk contains a number of samples with the same
		// stream characteristics, i.e. same sample rate, channel count
		// and channel configuration.
		virtual bool on_chunk(audio_chunk * chunk, abort_callback & p_abort) {
			if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
			{
				m_rate = chunk->get_srate();
				m_ch = chunk->get_channels();
				m_ch_mask = chunk->get_channel_config();
				m_buffers.set_count(0);
				m_buffers.set_count(m_ch);
				for (unsigned i = 0; i < m_ch; i++)
				{
					Vibrato & e = m_buffers[i];
					e.init(freq, depth, m_rate);
				}
			}

			for (unsigned i = 0; i < m_ch; i++)
			{
				Vibrato & e = m_buffers[i];
				audio_sample * data = chunk->get_data() + i;
				for (unsigned j = 0, k = chunk->get_sample_count(); j < k; j++)
				{
					*data = e.Process(*data);
					data += m_ch;
				}
			}
			return true;
		}

		virtual void flush() {
			m_buffers.set_count(0);
			m_rate = 0;
			m_ch = 0;
			m_ch_mask = 0;
		}

		virtual double get_latency() {
			// If the buffered chunk is valid, return its length.
			// Otherwise return 0.
			return 0.0;
		}

		virtual bool need_track_change_mark() {
			// Return true if you need to know exactly when a new track starts.
			// Beware that this may break gapless playback, as at least all the
			// DSPs before yours have to be flushed.
			// To picture this, consider the case of a reverb DSP which outputs
			// the sum of the input signal and a delayed copy of the input signal.
			// In the case of a single track:

			// Input signal:   01234567
			// Delayed signal:   01234567

			// For two consecutive tracks with the same stream characteristics:

			// Input signal:   01234567abcdefgh
			// Delayed signal:   01234567abcdefgh

			// If the DSP chain contains a DSP that requires a track change mark,
			// the chain will be flushed between the two tracks:

			// Input signal:   01234567  abcdefgh
			// Delayed signal:   01234567  abcdefgh
			return false;
		}
		static bool g_get_default_preset(dsp_preset & p_out)
		{
			make_preset(5., 0.5, true, p_out);
			return true;
		}
		static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
		{
			::RunConfigPopup(p_data, p_parent, p_callback);
		}
		static bool g_have_config_popup() { return true; }
		static void make_preset(float freq, float depth, bool enabled, dsp_preset & out)
		{
			dsp_preset_builder builder;
			builder << freq;
			builder << depth;
			builder << enabled;
			builder.finish(g_get_guid(), out);
		}
		static void parse_preset(float & freq, float & depth, bool & enabled, const dsp_preset & in)
		{
			try
			{
				dsp_preset_parser parser(in);
				parser >> freq;
				parser >> depth;
				parser >> enabled;
			}
			catch (exception_io_data) { freq = 2.; depth = 0.5; enabled = true; }
		}
	};

	// We need a service factory to make the DSP known to the system.
	// DSPs use special service factories that implement a static dsp_entry
	// that provides information about the DSP. The static methods in our
	// DSP class our used to provide the implementation of this entry class.
	// The entry is used to instantiate an instance of our DSP when it is needed.
	// We use the "nopreset" version of the DSP factory which blanks out
	// preset support.
	// Note that there can be multiple instances of a DSP which are used in
	// different threads.
	static dsp_factory_t<dsp_vibrato> foo_dsp_tutorial_nopreset;

	class CMyDSPPopupVibrato : public CDialogImpl<CMyDSPPopupVibrato>
	{
	public:
		CMyDSPPopupVibrato(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }
		enum { IDD = IDD_VIBRATO };
		enum
		{
			FreqMin = 200,
			FreqMax = 2000,
			FreqRangeTotal = FreqMax - FreqMin,
			depthmin = 0,
			depthmax = 100,
		};

		BEGIN_MSG_MAP(CMyDSPPopup)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
			MSG_WM_HSCROLL(OnHScroll)
		END_MSG_MAP()

	private:
		BOOL OnInitDialog(CWindow, LPARAM)
		{
			slider_freq = GetDlgItem(IDC_VIBRATOFREQ);
			slider_freq.SetRange(FreqMin, FreqMax);
			slider_depth = GetDlgItem(IDC_VIBRATODEPTH);
			slider_depth.SetRange(0, depthmax);
			{
				bool enabled;
				dsp_vibrato::parse_preset(freq, depth, enabled, m_initData);
				slider_freq.SetPos((double)(100 * freq));
				slider_depth.SetPos((double)(100 * depth));
				RefreshLabel(freq, depth);
			}

			return TRUE;
		}

		void OnButton(UINT, int id, CWindow)
		{
			EndDialog(id);
		}

		void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
		{

			freq = slider_freq.GetPos() / 100.0;
			depth = slider_depth.GetPos() / 100.0;
			if (LOWORD(nSBCode) != SB_THUMBTRACK)
			{
				dsp_preset_impl preset;
				dsp_vibrato::make_preset(freq, depth, true, preset);
				m_callback.on_preset_changed(preset);
			}
			RefreshLabel(freq, depth);

		}

		void RefreshLabel(float freq, float depth)
		{
			pfc::string_formatter msg;
			msg << "Frequency: ";
			msg << pfc::format_float(freq, 0, 2) << " Hz";
			::uSetDlgItemText(*this, IDC_VIBRATOFREQLAB, msg);
			msg.reset();
			msg << "Wet/Dry Mix : ";
			msg << pfc::format_int(100 * depth) << " %";
			::uSetDlgItemText(*this, IDC_VIBRATODEPTHLAB, msg);

		}

		const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
		dsp_preset_edit_callback & m_callback;
		float freq; //0.1 - 4.0
		float depth;  //0 - 360
		CTrackBarCtrl slider_freq, slider_depth;
	};

	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		CMyDSPPopupVibrato popup(p_data, p_callback);
		if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
	}

	// {1DC17CA0-0023-4266-AD59-691D566AC291}
	static const GUID guid_choruselem =
	{ 0x715a5d0c, 0x5eb2, 0x49c2,{ 0xa2, 0xfb, 0xc7, 0xb2, 0x30, 0x8, 0xe2, 0x50 } };




	class uielem_vibrato : public CDialogImpl<uielem_vibrato>, public ui_element_instance {
	public:
		uielem_vibrato(ui_element_config::ptr cfg, ui_element_instance_callback::ptr cb) : m_callback(cb) {
			freq = 5.0;
			depth = 0.5;
			echo_enabled = true;

		}
		enum { IDD = IDD_VIBRATO1 };
		enum
		{
			FreqMin = 200,
			FreqMax = 2000,
			FreqRangeTotal = FreqMax - FreqMin,
			depthmin = 0,
			depthmax = 100,
		};
		BEGIN_MSG_MAP(uielem_vibrato)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDC_VIBRATOENABLED, BN_CLICKED, OnEnabledToggle)
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
		static void g_get_name(pfc::string_base & out) { out = "Vibrato"; }
		static ui_element_config::ptr g_get_default_configuration() {
			return makeConfig(true);
		}
		static const char * g_get_description() { return "Modifies the 'Vibrato' DSP effect."; }
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
			ret.m_min_height = MulDiv(150, DPI.cy, 96);
			ret.m_max_width = MulDiv(380, DPI.cx, 96);
			ret.m_max_height = MulDiv(150, DPI.cy, 96);

			// Deal with WS_EX_STATICEDGE and alike that we might have picked from host
			ret.adjustForWindow(*this);

			return ret;
		}

	private:
		void SetEchoEnabled(bool state) { m_buttonEchoEnabled.SetCheck(state ? BST_CHECKED : BST_UNCHECKED); }
		bool IsEchoEnabled() { return m_buttonEchoEnabled == NULL || m_buttonEchoEnabled.GetCheck() == BST_CHECKED; }

		void EchoDisable() {
			static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_vibrato);
		}


		void EchoEnable(float freq, float  depth, bool echo_enabled) {
			dsp_preset_impl preset;
			dsp_vibrato::make_preset(freq, depth, echo_enabled, preset);
			static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
		}

		void OnEnabledToggle(UINT uNotifyCode, int nID, CWindow wndCtl) {
			pfc::vartoggle_t<bool> ownUpdate(m_ownEchoUpdate, true);
			if (IsEchoEnabled()) {
				GetConfig();
				dsp_preset_impl preset;
				dsp_vibrato::make_preset(freq, depth, echo_enabled, preset);
				//yes change api;
				static_api_ptr_t<dsp_config_manager>()->core_enable_dsp(preset, dsp_config_manager::default_insert_last);
			}
			else {
				static_api_ptr_t<dsp_config_manager>()->core_disable_dsp(guid_vibrato);
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
					EchoEnable(freq, depth, echo_enabled);
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
			if (static_api_ptr_t<dsp_config_manager>()->core_query_dsp(guid_vibrato, preset)) {
				SetEchoEnabled(true);
				dsp_vibrato::parse_preset(freq, depth, echo_enabled, preset);
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
				EchoEnable(freq, depth, echo_enabled);
			}
			else {
				EchoDisable();
			}

		}


		void GetConfig()
		{
			freq = slider_freq.GetPos() / 100.0;
			depth = slider_depth.GetPos() / 100.0;
			echo_enabled = IsEchoEnabled();
			RefreshLabel(freq, depth);


		}

		void SetConfig()
		{
			slider_freq.SetPos((double)(100 * freq));
			slider_depth.SetPos((double)(100 * depth));

			RefreshLabel(freq, depth);

		}

		BOOL OnInitDialog(CWindow, LPARAM)
		{

			slider_freq = GetDlgItem(IDC_VIBRATOFREQ1);
			slider_freq.SetRange(FreqMin, FreqMax);
			slider_depth = GetDlgItem(IDC_VIBRATODEPTH1);
			slider_depth.SetRange(0, depthmax);


			m_buttonEchoEnabled = GetDlgItem(IDC_VIBRATOENABLED);
			m_ownEchoUpdate = false;

			ApplySettings();
			return TRUE;
		}


		void RefreshLabel(float freq, float depth)
		{
			pfc::string_formatter msg;
			msg << "Frequency: ";
			msg << pfc::format_float(freq, 0, 2) << " Hz";
			::uSetDlgItemText(*this, IDC_VIBRATOFREQLAB1, msg);
			msg.reset();
			msg << "Wet/Dry Mix : ";
			msg << pfc::format_int(100 * depth) << " %";
			::uSetDlgItemText(*this, IDC_VIBRATODEPTHLAB1, msg);
		}

		bool echo_enabled;
		float freq; //0.1 - 4.0
		float depth;  //0 - 360
		CTrackBarCtrl slider_freq, slider_depth;
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

	class myElem_t : public  ui_element_impl_withpopup< uielem_vibrato > {
		bool get_element_group(pfc::string_base & p_out)
		{
			p_out = "Effect DSP";
			return true;
		}

		bool get_menu_command_description(pfc::string_base & out) {
			out = "Opens a window for pitch modulation control.";
			return true;
		}

	};
	static service_factory_single_t<myElem_t> g_myElemFactory;

}
