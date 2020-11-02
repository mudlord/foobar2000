#include <math.h>
#include "../SDK/foobar2000.h"
#include "../helpers/helpers.h"
#include "resource.h"
#include "freeverb.h"
#include <windows.h>
// {45D801B0-4880-4bb2-8A28-6E0966CAEC58}
static const GUID guid = 
{ 0x45d801b0, 0x4880, 0x4bb2, { 0x8a, 0x28, 0x6e, 0x9, 0x66, 0xca, 0xec, 0x58 } };

DECLARE_COMPONENT_VERSION(
	"Freeverb",
	"0.3",
	"A reverberation DSP for foobar2000 1.0 ->\n"
	"Written by mudlord\n"
	"http://mudlord.hcs64.com\n"
	"Based on Freeverb by Jezar Wakefield\n"
	"Portions by Jon Watte\n"
);

VALIDATE_COMPONENT_FILENAME("foo_dsp_freeverb.dll");

struct t_dsp_reverb_params
{
	float drytime;
	float wettime;
    float dampness;
	float roomwidth;
	float roomsize;
	
	float reverbtime;
	t_dsp_reverb_params() { drytime = 0.43; wettime = 0.57; 
	dampness = 0.45;
	roomwidth = 0.56;
	roomsize = 0.56;
	};

	// Read in data from a preset.
	bool set_data(const dsp_preset & p_data)
	{
		if (p_data.get_data_size() != sizeof(t_dsp_reverb_params)) return false;
		t_dsp_reverb_params temp = *(t_dsp_reverb_params *)p_data.get_data();
		byte_order::order_le_to_native_t(temp);
		drytime = temp.drytime; // Set it here (get it from the passed arg)
		wettime = temp.wettime;
		dampness = temp.dampness;
		roomwidth = temp.roomwidth;
		roomsize = temp.roomsize;

		return true;
	}

	bool get_data(dsp_preset & p_data)
	{
		t_dsp_reverb_params temp;
		temp.drytime = drytime;
		temp.wettime = wettime;
		temp.dampness = dampness;
		temp.roomwidth = roomwidth;
		temp.roomsize = roomsize;
		byte_order::order_native_to_le_t(temp);
		p_data.set_data(&temp, sizeof(temp)); // Get it here (set it in the passed arg)
		return true;
	}
};


class  dialog_dsp_reverb : public dialog_helper::dialog_modal {
public:
	dialog_dsp_reverb(t_dsp_reverb_params & p_params, dsp_preset_edit_callback & p_callback) : m_params(p_params), m_callback(p_callback) {	}
	virtual BOOL on_message(UINT msg, WPARAM wp, LPARAM lp)
	{
		switch(msg)
		{
		case WM_INITDIALOG:
			{
				HWND slider1 = GetDlgItem(get_wnd(), IDC_WET);
				uSendMessage(slider1, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
				uSendMessage(slider1, TBM_SETPOS, TRUE, (double)(100*m_params.wettime));
				HWND slider2 = GetDlgItem(get_wnd(), IDC_DRY);
				uSendMessage(slider2, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
				uSendMessage(slider2, TBM_SETPOS, TRUE, (double)(100*m_params.drytime));
				HWND slider3 = GetDlgItem(get_wnd(), IDC_DAMP);
				uSendMessage(slider3, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
				uSendMessage(slider3, TBM_SETPOS, TRUE, (double)(100*m_params.dampness));
				HWND slider4 = GetDlgItem(get_wnd(), IDC_WIDTH);
				uSendMessage(slider4, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
				uSendMessage(slider4, TBM_SETPOS, TRUE, (double)(100*m_params.roomwidth));
				HWND slider5 = GetDlgItem(get_wnd(), IDC_SIZE);
				uSendMessage(slider5, TBM_SETRANGE, TRUE, MAKELONG(0, 100));
				uSendMessage(slider5, TBM_SETPOS, TRUE, (double)(100*m_params.roomsize));
				
				changed_since_saved = false;
				saved = false;
				
				update_slider_display();

				// Set revert dsp settings MORE
				rev_params.wettime = m_params.wettime;
				rev_params.drytime = m_params.drytime;
				rev_params.dampness = m_params.dampness;
				rev_params.roomwidth = m_params.roomwidth;
				rev_params.roomsize = m_params.roomsize;
				
			}
			break;

			// Slider has been moved.
		case WM_HSCROLL:
			{
				changed_since_saved = true;
				m_params.wettime = (double)uSendDlgItemMessage(get_wnd(), IDC_WET, TBM_GETPOS, 0, 0)/100.0;
				m_params.drytime = (double)uSendDlgItemMessage(get_wnd(), IDC_DRY, TBM_GETPOS, 0, 0)/100.0;
				m_params.dampness = (double)uSendDlgItemMessage(get_wnd(), IDC_DAMP, TBM_GETPOS, 0, 0)/100.0;
				m_params.roomsize = (double)uSendDlgItemMessage(get_wnd(), IDC_SIZE, TBM_GETPOS, 0, 0)/100.0;
				m_params.roomwidth = (double)uSendDlgItemMessage(get_wnd(), IDC_WIDTH, TBM_GETPOS, 0, 0)/100.0;
				
				
				update_slider_display();
				UpdatePreset();
				saved = true;
				changed_since_saved = false;
			}
			break;

		case WM_CLOSE:
			{
				// Data not changed
				end_dialog(0);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wp))
			{

			}
			break;
		}
		return 0;
	}

private:
	bool changed_since_saved;
	bool saved;
	t_dsp_reverb_params & m_params;
	t_dsp_reverb_params rev_params;
	dsp_preset_edit_callback & m_callback;

	static bool g_get_data(dsp_preset & p_data, t_dsp_reverb_params & tparams)
	{
		p_data.set_owner(guid);
		return tparams.get_data(p_data);
	}

	void UpdatePreset() {
		dsp_preset_impl temp;
		g_get_data(temp, m_params);
		m_callback.on_preset_changed(temp);
	}
	
	void update_slider_display()
	{
		char temp[128];
		sprintf(temp,"%1.2f",((double)uSendDlgItemMessage(get_wnd(),IDC_WET,TBM_GETPOS,0,0)) / 100); 
		uSetDlgItemText(get_wnd(), IDC_WETVALUE,temp);
		sprintf(temp,"%1.2f",((double)uSendDlgItemMessage(get_wnd(),IDC_DRY,TBM_GETPOS,0,0)) / 100); 
		uSetDlgItemText(get_wnd(), IDC_DRYVALUE,temp);
		sprintf(temp,"%1.2f",((double)uSendDlgItemMessage(get_wnd(),IDC_DAMP,TBM_GETPOS,0,0)) / 100); 
		uSetDlgItemText(get_wnd(), IDC_DAMPVALUE,temp);
		sprintf(temp,"%1.2f",((double)uSendDlgItemMessage(get_wnd(),IDC_SIZE,TBM_GETPOS,0,0)) / 100); 
		uSetDlgItemText(get_wnd(), IDC_SIZEVALUE,temp);
		sprintf(temp,"%1.2f",((double)uSendDlgItemMessage(get_wnd(),IDC_WIDTH,TBM_GETPOS,0,0)) / 100); 
		uSetDlgItemText(get_wnd(), IDC_WIDTHVALUE,temp);
	}
};



class dsp_reverb : public dsp_impl_base {
	t_dsp_reverb_params Params;
	revmodel Freeverb;

public:
	dsp_reverb(const dsp_preset & p_data) {
		set_data(p_data);
		Freeverb.setwet(Params.wettime);
		Freeverb.setdry(Params.drytime);
		Freeverb.setdamp(Params.dampness);
		Freeverb.setroomsize(Params.roomsize);
		Freeverb.setwidth(Params.roomwidth);
	}

	~dsp_reverb() {
	}

	static GUID g_get_guid() {
		return guid;
	}

	static void g_get_name(pfc::string_base & p_out) {
		p_out = "Freeverb DSP";
	}

	static bool g_have_config_popup() {
		return true;
	}

	static void g_show_config_popup(const dsp_preset & p_data, HWND p_parent, dsp_preset_edit_callback & p_callback) {
		t_dsp_reverb_params params;       // Create a params variable
		if (!params.set_data(p_data)) return; // Set params to the data in p_data
		dialog_dsp_reverb dlg(params,p_callback);    // Create a dialog using params
		dlg.run(IDD_FREEDIALOG, p_parent);        // Run a dialog putting data into params
	}

	// Return the default preset
	static bool g_get_default_preset(dsp_preset & p_out) {
		t_dsp_reverb_params tparams;
		return g_get_data(p_out, tparams);
	}

	// Read parameters from a provided preset
	bool set_data(const dsp_preset & p_data)
	{
		if (!g_set_data(p_data, Params)) return false;
		return true;
	}

	// Helper method for reading parameters from a preset.
	static bool g_set_data(const dsp_preset & p_data, t_dsp_reverb_params & outparams) {
		if (p_data.get_owner() == g_get_guid())
		{
			t_dsp_reverb_params inparams;
			if (!inparams.set_data(p_data)) return false;
			outparams.drytime = inparams.drytime;
			outparams.wettime = inparams.wettime;
			outparams.dampness = inparams.dampness;
			outparams.roomwidth = inparams.roomwidth;
			outparams.roomsize = inparams.roomsize;
			return true;
		}
		return false;
	}

	// Helper method for writing parameters to a preset.
	static bool g_get_data(dsp_preset & p_data, t_dsp_reverb_params & tparams)
	{
		p_data.set_owner(g_get_guid());
		return tparams.get_data(p_data);
	}

	virtual bool on_chunk(audio_chunk * chunk, abort_callback &) {
		t_size sample_count = chunk->get_sample_count();
		audio_sample * current = chunk->get_data();
		unsigned channel_count = chunk->get_channels();

		switch(channel_count)
		{
		case 1:
			Freeverb.processmono((float*)current,sample_count);
			break;
		case 2: 
			Freeverb.processstereo((float*)current,sample_count);
			break;
		}
		return true;
	}

	virtual void flush() {
		// Nothing to flush.
	}

	virtual void on_endofplayback(abort_callback &) {
		// This method is called on end of playback instead of flush().
		// We need to do the same thing as flush(), so we just call it.
		flush();
	}

	virtual double get_latency() {
		// If the buffered chunk is valid, return its length.
		// Otherwise return 0.
		return 0.0;
	}
	virtual void on_endoftrack(abort_callback &) {}
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
};

static dsp_factory_t<dsp_reverb> foo_dsp_reverb;


