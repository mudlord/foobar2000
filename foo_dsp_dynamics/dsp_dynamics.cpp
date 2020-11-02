#define _WIN32_WINNT 0x0501
#include "../SDK/foobar2000.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "resource.h"
#include <math.h>
#include <commctrl.h>
/*
* Normally, elements with number suffices are progressively
* slower.
*/


namespace {

	// {E1746679-F1C6-48D0-AFE0-CB7038454CF1}
	static const GUID guid_dynamics =
	{ 0xe1746679, 0xf1c6, 0x48d0,{ 0xaf, 0xe0, 0xcb, 0x70, 0x38, 0x45, 0x4c, 0xf1 } };

#define NFILT 12
#define NEFILT 17
	struct compstruct {
		/* Simple level running average */
		double rlevelsq0, rlevelsq1;
		double rlevelsq0filter, rlevelsq1filter;
		double rlevelsqn[NFILT];
		double rlevelsqefilter;
		double rlevelsqe[NEFILT];
		double rlevelsq0ffilter;
		int ndelay; /* delay for rlevelsq0ffilter delay */
		int ndelayptr; /* ptr for the input */
		double *rightdelay;
		double *leftdelay;
		/* Simple gain running average */
		double rgain;
		double rgainfilter;
		double lastrgain;
		/* Max fast agc gain, slow agc gain */
		double maxfastgain, maxslowgain;
		/* Fast gain compression ratio */
		/* Note that .5 is 2:1, 1.0 is infinity (hard) */
		double fastgaincompressionratio;
		double compressionratio;
		/* Max level, target level, floor level */
		double maxlevel, targetlevel, floorlevel;
		/* Gainriding gain */
		double rmastergain0filter;
		double rmastergain0;
		/* Peak limit gain */
		double rpeakgain0, rpeakgain1, rpeakgainfilter;
		int peaklimitdelay, rpeaklimitdelay;
		/* Running total gain */
		double totalgain;
		/* Idle gain */
		double npeakgain;
		/* Compress enabled */
		int compress;
	};

	__inline double hardlimit(double value, double knee, double limit)
	{
		double lrange = (limit - knee);
		double ab = fabs(value);

		if (ab > knee) {
			double abslimit = (limit * 1.1);
			if (ab < abslimit)
				value = knee + lrange * sin(((value - knee) / abslimit) * (3.14 / (4 * 1.1)));
		}

		if (ab >= limit)
			value = value > 0 ? limit : -limit;
		return value;
	}


	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback);
	class dsp_dynamics : public dsp_impl_base
	{
		int m_rate, m_ch, m_ch_mask;
		float peaklimit;
		float releasetime;
		float fastratio;
		float slowratio;
		float gain;
		bool dynamics_enabled;
		compstruct state;
	public:
		dsp_dynamics(dsp_preset const & in) : peaklimit(0.90), releasetime(0.30), fastratio(0.25), slowratio(0.50), gain(1.0), m_rate(0), m_ch(0), m_ch_mask(0)
		{
			dynamics_enabled = true;
			parse_preset(peaklimit, releasetime, fastratio, slowratio, gain, dynamics_enabled, in);
			recalculate_parameters();
		}

		~dsp_dynamics()
		{
			if (state.rightdelay)
			{
				free(state.rightdelay);
				state.rightdelay = NULL;
			}
			if (state.leftdelay)
			{
				free(state.leftdelay);
				state.leftdelay = NULL;
			}
		}

		static GUID g_get_guid()
		{

			return guid_dynamics;
		}

		static void g_get_name(pfc::string_base & p_out) { p_out = "Dynamics Compressor"; }

		bool on_chunk(audio_chunk * chunk, abort_callback &)
		{
			double dright, dleft;

			if (!dynamics_enabled) return true;

			if (chunk->get_srate() != m_rate || chunk->get_channels() != m_ch || chunk->get_channel_config() != m_ch_mask)
			{
				m_rate = chunk->get_srate();
				m_ch = chunk->get_channels();
				m_ch_mask = chunk->get_channel_config();
				recalculate_parameters();
			}

			// design is for 44.1kHz, allow anything from 30kHz to 50kHz
			// and handle mono
			if (m_rate > 30000 && m_rate < 50000)
			{
				if (m_ch == 2)
				{
					unsigned int n;
					for (n = 0; n < chunk->get_sample_count() << 1; n += 2)
					{
						dleft = chunk->get_data()[n];
						dright = chunk->get_data()[n + 1];

						// 16 bit levels are needed 
						// for compressor code
						dleft *= gain * 32767.0;
						dright *= gain * 32767.0;
						if (state.compress)
							compressor(&dright, &dleft);
						chunk->get_data()[n] = dleft / 32767.0;
						chunk->get_data()[n + 1] = dright / 32767.0;
					}
				}
				else if (m_ch == 1)
				{
					unsigned int n;
					for (n = 0; n < chunk->get_sample_count(); n++)
					{
						dleft = chunk->get_data()[n];

						// 16 bit levels are needed 
						// for compressor code
						dleft *= gain * 32767.0;
						dright = dleft;
						if (state.compress)
							compressor(&dright, &dleft);
						chunk->get_data()[n] = dleft / 32767.0;
					}

				}
			}
			return true;
		}


		void on_endofplayback(abort_callback &) { }
		void on_endoftrack(abort_callback &) { }

		void flush()
		{
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
			make_preset(0.90, 0.30, 0.25, 0.50, 1.0, true, p_out);
			return true;
		}

		static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
		{
			::RunConfigPopup(p_data, p_parent, p_callback);
		}

		static bool g_have_config_popup() { return true; }
		static void make_preset(float peaklimit, float  releasetime, float fastratio, float slowratio, float gain, bool enabled, dsp_preset & out)
		{
			dsp_preset_builder builder;
			builder << peaklimit;
			builder << releasetime;
			builder << fastratio;
			builder << slowratio;
			builder << gain;
			builder << enabled;
			builder.finish(g_get_guid(), out);
		}
		static void parse_preset(float & peaklimit, float & releasetime, float & fastratio, float & slowratio, float & gain, bool & enabled, const dsp_preset & in)
		{
			try
			{
				dsp_preset_parser parser(in);
				parser >> peaklimit;
				parser >> releasetime;
				parser >> fastratio;
				parser >> slowratio;
				parser >> gain;
				parser >> enabled;
			}
			catch (exception_io_data) { peaklimit = 0.90; releasetime = 0.30; fastratio = 0.25; slowratio = 0.50; gain = 1.0; enabled = true; }
		}

	private:
		void recalculate_parameters(void) {
			int i;
			memset(&state, 0, sizeof(compstruct));


			/* These filters should filter at least the lowest audio freq */
			state.rlevelsq0filter = .001;
			state.rlevelsq1filter = .010;
			/* These are the attack time for the rms measurement */
			state.rlevelsq0ffilter = .001;

			state.rlevelsqefilter = .001;

			/*
			* Linear gain filters as opposed to the level measurement filters
			* above
			*/
			if (releasetime < 0.01)
				releasetime = 0.01;
			state.rgainfilter = 1.0 / (releasetime * 44100.0);

			/*
			* compression ratio for fast gain.  This will determine how
			* much the audio is made more dense.  .5 is equiv to 2:1
			* compression.  1.0 is equiv to inf:1 compression.
			*/
			state.fastgaincompressionratio = fastratio;
			state.compressionratio = slowratio;

			/*
			* Limiter level
			*/
			state.maxlevel = 32000;
			/*
			* maximum gain for fast compressor
			*/
			state.maxfastgain = 3;
			/*
			* maximum gain for slow compressor
			*/
			state.maxslowgain = 9;
			/*
			* target level for compression
			*/
			state.targetlevel = state.maxlevel * peaklimit;
			/*
			* Level below which gain tracking shuts off
			*/
			state.floorlevel = 2000;

			/*
			* Slow compressor time constants
			*/
			state.rmastergain0filter = .000003;

			state.rpeakgainfilter = .001;
			state.rpeaklimitdelay = 2500;

			state.rgain = state.rmastergain0 = 1.0;
			state.rlevelsq0 = state.rlevelsq1 = 0;
			state.compress = 1;
			state.ndelay = 1.0 / state.rlevelsq0ffilter;
			state.rightdelay = (double*)calloc(state.ndelay, sizeof(double));
			state.leftdelay = (double*)calloc(state.ndelay, sizeof(double));
			state.rpeakgain0 = 1.0;
			state.rpeakgain1 = 1.0;
			state.rpeaklimitdelay = 0;
			state.ndelayptr = 0;
			state.lastrgain = 1.0;
			for (i = 0; i < NFILT; i++)
				state.rlevelsqn[i] = 0;
			for (i = 0; i < NEFILT; i++)
				state.rlevelsqe[i] = 0;
		}

		void compressor(double *righta, double *lefta) {

			double levelsq0, levelsqe;
			double gain, qgain, tgain;
			double newright, newleft;
			double efilt;
			double fastgain, slowgain, tslowgain;
			double right, left, rightd, leftd;
			double nrgain, nlgain, ngain, ngsq;
			double sqrtrpeakgain;
			int i;
			int skipmode;

			right = *righta;
			left = *lefta;

			state.rightdelay[state.ndelayptr] = right;
			state.leftdelay[state.ndelayptr] = left;
			state.ndelayptr++;
			if (state.ndelayptr >= state.ndelay)
				state.ndelayptr = 0;
			/* enable/disable compression */

			skipmode = 0;
			if (state.compress == 0) {
				skipmode = 1;
				goto skipagc;
			}
			levelsq0 = (right) * (right)+(left) * (left);

			if (levelsq0 > state.rlevelsq0) {
				state.rlevelsq0 = (levelsq0 * state.rlevelsq0ffilter) +
					state.rlevelsq0 * (1 - state.rlevelsq0ffilter);
			}
			else {
				state.rlevelsq0 = (levelsq0 * state.rlevelsq0filter) +
					state.rlevelsq0 * (1 - state.rlevelsq0filter);
			}

			if (state.rlevelsq0 <= state.floorlevel * state.floorlevel)
				goto skipagc; /* no compression at low signal levels */

			if (state.rlevelsq0 > state.rlevelsq1) {
				state.rlevelsq1 = state.rlevelsq0;
			}
			else {
				state.rlevelsq1 = state.rlevelsq0 * state.rlevelsq1filter +
					state.rlevelsq1 * (1 - state.rlevelsq1filter);
			}
			/* that was the decay */

			state.rlevelsqn[0] = state.rlevelsq1;
			for (i = 0; i < NFILT - 1; i++) {
				if (state.rlevelsqn[i] > state.rlevelsqn[i + 1])
					state.rlevelsqn[i + 1] = state.rlevelsqn[i];
				else
					state.rlevelsqn[i + 1] = state.rlevelsqn[i] * state.rlevelsq1filter +
					state.rlevelsqn[i + 1] * (1 - state.rlevelsq1filter);
			}


			efilt = state.rlevelsqefilter;
			levelsqe = state.rlevelsqe[0] = state.rlevelsqn[NFILT - 1];
			for (i = 0; i < NEFILT - 1; i++) {
				state.rlevelsqe[i + 1] = state.rlevelsqe[i] * efilt +
					state.rlevelsqe[i + 1] * (1.0 - efilt);
				if (state.rlevelsqe[i + 1] > levelsqe)
					levelsqe = state.rlevelsqe[i + 1];
				efilt *= 1.0 / 1.5;
			}

			gain = state.targetlevel / sqrt(levelsqe);
			if (state.compressionratio < 0.99) {
				if (state.compressionratio == 0.50)
					gain = sqrt(gain);
				else
					gain = exp(log(gain) * state.compressionratio);
			}

			if (gain < state.rgain)
				state.rgain = gain * state.rlevelsqefilter / 2 +
				state.rgain * (1 - state.rlevelsqefilter / 2);
			else
				state.rgain = gain * state.rgainfilter +
				state.rgain * (1 - state.rgainfilter);

			state.lastrgain = state.rgain;
			if (gain < state.lastrgain)
				state.lastrgain = gain;

		skipagc:;

			tgain = state.lastrgain;

			leftd = state.leftdelay[state.ndelayptr];
			rightd = state.rightdelay[state.ndelayptr];

			fastgain = tgain;
			if (fastgain > state.maxfastgain)
				fastgain = state.maxfastgain;

			if (fastgain < 0.0001)
				fastgain = 0.0001;

			if (state.fastgaincompressionratio == 0.25) {
				qgain = sqrt(sqrt(fastgain));
			}
			else if (state.fastgaincompressionratio == 0.5) {
				qgain = sqrt(fastgain);
			}
			else if (state.fastgaincompressionratio == 1.0) {
				qgain = fastgain;
			}
			else {
				qgain = exp(log(fastgain) * state.fastgaincompressionratio);
			}

			tslowgain = tgain / qgain;
			if (tslowgain > state.maxslowgain)
				tslowgain = state.maxslowgain;
			if (tslowgain < state.rmastergain0)
				state.rmastergain0 = tslowgain;
			else
				state.rmastergain0 = tslowgain * state.rmastergain0filter +
				(1 - state.rmastergain0filter) * state.rmastergain0;

			slowgain = state.rmastergain0;
			if (skipmode == 0)
				state.npeakgain = slowgain * qgain;

			/**/
			newright = rightd * state.npeakgain;
			if (fabs(newright) >= state.maxlevel)
				nrgain = state.maxlevel / fabs(newright);
			else
				nrgain = 1.0;

			newleft = leftd * state.npeakgain;
			if (fabs(newleft) >= state.maxlevel)
				nlgain = state.maxlevel / fabs(newleft);
			else
				nlgain = 1.0;

			ngain = nrgain;
			if (nlgain < ngain)
				ngain = nlgain;

			ngsq = ngain * ngain;
			if (ngsq <= state.rpeakgain0) {
				state.rpeakgain0 = ngsq /* * 0.50 + cs->rpeakgain0 * 0.50 */;
				state.rpeaklimitdelay = state.peaklimitdelay;
			}
			else if (state.rpeaklimitdelay == 0) {
				double tnrgain;
				if (nrgain > 1.0)
					tnrgain = 1.0;
				else
					tnrgain = nrgain;
				state.rpeakgain0 = tnrgain * state.rpeakgainfilter +
					(1.0 - state.rpeakgainfilter) * state.rpeakgain0;
			}

			if (state.rpeakgain0 <= state.rpeakgain1) {
				state.rpeakgain1 = state.rpeakgain0;
				state.rpeaklimitdelay = state.peaklimitdelay;
			}
			else if (state.rpeaklimitdelay == 0) {
				state.rpeakgain1 = state.rpeakgainfilter * state.rpeakgain0 +
					(1.0 - state.rpeakgainfilter) * state.rpeakgain1;
			}
			else {
				--state.rpeaklimitdelay;
			}

			sqrtrpeakgain = sqrt(state.rpeakgain1);
			state.totalgain = state.npeakgain * sqrtrpeakgain;

			right = newright * sqrtrpeakgain;
			*righta = hardlimit(right, 32200, 32767);

			left = newleft * sqrtrpeakgain;
			*lefta = hardlimit(left, 32200, 32767);
		}
	};

	static dsp_factory_t<dsp_dynamics> dsp_dynamics_factory;

	static const GUID guid_cfg_placement =
	{ 0x6ce1ca13, 0xe1ba, 0x41de,{ 0x8b, 0xe0, 0x11, 0xac, 0x9c, 0x61, 0x82, 0xbe } };

	static cfg_window_placement cfg_placement(guid_cfg_placement);


	class CMyDSPPopupDynamics : public CDialogImpl<CMyDSPPopupDynamics>
	{
	public:
		CMyDSPPopupDynamics(const dsp_preset & initData, dsp_preset_edit_callback & callback) : m_initData(initData), m_callback(callback) { }
		enum { IDD = IDD_DYNAMICS };

		enum
		{
			RANGE = 1000
		};

		BEGIN_MSG_MAP(CMyDSPPopupDynamics)
			MSG_WM_INITDIALOG(OnInitDialog)
			COMMAND_HANDLER_EX(IDOK, BN_CLICKED, OnButton)
			COMMAND_HANDLER_EX(IDCANCEL, BN_CLICKED, OnButton)
			MSG_WM_HSCROLL(OnHScroll)
		END_MSG_MAP()

	private:

		BOOL OnInitDialog(CWindow, LPARAM)
		{
			slider_peaklimit = GetDlgItem(IDC_DYNAMICSPEAKLIMIT);
			slider_peaklimit.SetRange(0, RANGE);
			slider_releasetime = GetDlgItem(IDC_DYNAMICSRELEASETIME);
			slider_releasetime.SetRange(0, RANGE);
			slider_fastratio = GetDlgItem(IDC_DYNAMICSFASTRATIO);
			slider_fastratio.SetRange(0, RANGE);
			slider_slowratio = GetDlgItem(IDC_DYNAMICSSLOWRATIO);
			slider_slowratio.SetRange(0, RANGE);
			slider_gain = GetDlgItem(IDC_DYNAMICSGAIN);
			slider_gain.SetRange(0, 600);
			{

				bool dynamics_enabled;
				dsp_dynamics::parse_preset(peaklimit, releasetime, fastratio, slowratio, gain, dynamics_enabled, m_initData);
				slider_peaklimit.SetPos((double)(1000 * peaklimit));
				slider_releasetime.SetPos((double)(1000 * releasetime));
				slider_fastratio.SetPos((double)(1000 * fastratio));
				slider_slowratio.SetPos((double)(1000 * slowratio));
				slider_gain.SetPos((double)((gain - 1.0)*200.0));
				RefreshLabel(peaklimit, releasetime, fastratio, slowratio, gain);
			}
			return TRUE;
		}

		void OnButton(UINT, int id, CWindow)
		{
			EndDialog(id);
		}

		void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar pScrollBar)
		{
			peaklimit = slider_peaklimit.GetPos() / 1000.0;
			releasetime = slider_releasetime.GetPos() / 1000.0;
			fastratio = slider_fastratio.GetPos() / 1000.0;
			slowratio = slider_slowratio.GetPos() / 1000.0;
			gain = slider_gain.GetPos() / 200.0;
			gain = 1.0 + (gain);
			if (LOWORD(nSBCode) != SB_THUMBTRACK)
			{
				dsp_preset_impl preset;
				dsp_dynamics::make_preset(peaklimit, releasetime, fastratio, slowratio, gain, true, preset);
				m_callback.on_preset_changed(preset);
			}
			RefreshLabel(peaklimit, releasetime, fastratio, slowratio, gain);
		}

		void RefreshLabel(float peaklimit, float  releasetime, float fastratio, float slowratio, float gain)
		{
			float fval;
			pfc::string8 temp;
			pfc::string_formatter msg;
			msg << "Peak Limit: ";
			msg << pfc::format_int(peaklimit * 100) << "%";
			::uSetDlgItemText(*this, IDC_DYNAMICSDISPLAY_PEAK, msg);
			msg.reset();
			msg << "Release Time: ";
			msg << pfc::format_int(releasetime * 1000) << " ms";
			::uSetDlgItemText(*this, IDC_DYNAMICSDISPLAY_RELEASE, msg);
			msg.reset();
			msg << "Fast Ratio: ";
			msg << pfc::format_int(fastratio * 100) << "%";
			::uSetDlgItemText(*this, IDC_DYNAMICSDISPLAY_FASTRATIO, msg);
			msg.reset();
			msg << "Slow Ratio: ";
			msg << pfc::format_int(slowratio * 100) << "%";
			::uSetDlgItemText(*this, IDC_DYNAMICSDISPLAY_SLOWRATIO, msg);
			msg.reset();
			float gain_val = (gain - 1.0) * 200;
			float final = ((gain_val / 600) * 100);
			msg << "Input Gain: ";
			msg << pfc::format_int(final) << "%";
			::uSetDlgItemText(*this, IDC_DYNAMICSDISPLAY_GAIN, msg);
		}
		float peaklimit;
		float releasetime;
		float fastratio;
		float slowratio;
		float gain;
		const dsp_preset & m_initData; // modal dialog so we can reference this caller-owned object.
		dsp_preset_edit_callback & m_callback;
		CTrackBarCtrl slider_peaklimit, slider_releasetime, slider_fastratio, slider_slowratio, slider_gain;
	};

	static void RunConfigPopup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback)
	{
		CMyDSPPopupDynamics popup(p_data, p_callback);
		if (popup.DoModal(p_parent) != IDOK) p_callback.on_preset_changed(p_data);
	}






#define MYVERSION "0.2.1"

	static pfc::string_formatter g_get_component_about()
	{
		pfc::string_formatter about;
		about << "A dynamics compressor DSP for foobar2000 1.4 ->\n";
		about << "Written by Gian-Carlo Pascutto.\n";
		about << "Port to foobar2000 1.4 by mudlord (mudlord@rebote.net).\n";
		return about;
	}

	DECLARE_COMPONENT_VERSION_COPY(
	"Dynamics Compressor",
		MYVERSION,
		g_get_component_about()
		);
	VALIDATE_COMPONENT_FILENAME("foo_dsp_dynamics.dll");


}