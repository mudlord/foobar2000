#include "targetver.h"
#include <Windows.h>
#include "../ATLHelpers/ATLHelpers.h"
#include "../shared/shared.h"
#include "resource.h"

using namespace pfc;
// {63CE9D3D-7FCD-4f06-9384-4B74008DBC74}
static const GUID guid = 
{ 0x63ce9d3d, 0x7fcd, 0x4f06, { 0x93, 0x84, 0x4b, 0x74, 0x0, 0x8d, 0xbc, 0x74 } };

VALIDATE_COMPONENT_FILENAME("foo_mircnp.dll");
DECLARE_COMPONENT_VERSION(
	"mIRC Now Playing Spammer",
	"0.1",
	"A mIRC now playing component for foobar2000 1.0 ->\n"
	"Copyright (C) 2010 Brad Miller\n"
	"http://mudlord.info\n"
	"Portions (C) Chris Moeller\n"
);

static const GUID cfg_format_guid = 
{ 0x17f3215e, 0x77f5, 0x4e97, { 0x8e, 0xd2, 0xed, 0x8e, 0x28, 0xbb, 0x5, 0x24 } };
static const GUID cfg_enabled_guid = 
{ 0xd199eb67, 0xef98, 0x4fc6, { 0x97, 0xc, 0x2e, 0x6c, 0x38, 0x32, 0x94, 0x5 } };
static cfg_string cfg_format(cfg_format_guid,"%title% - %album%");
static cfg_bool cfg_enabled(cfg_enabled_guid,true);

class CMyPreferences : public CDialogImpl<CMyPreferences>, public preferences_page_instance {
public:
	CMyPreferences(preferences_page_callback::ptr callback) : m_callback(callback) {}
	//dialog resource ID
	enum {IDD = IDD_DIALOG};
	// preferences_page_instance methods (not all of them - get_wnd() is supplied by preferences_page_impl helpers)
	t_uint32 get_state();
	void apply();
	void reset();

	//WTL message map
	BEGIN_MSG_MAP(CMyPreferences)
		MSG_WM_INITDIALOG(OnInitDialog)
		COMMAND_HANDLER_EX(IDC_ENABLED, EN_CHANGE, OnEditChange)
		COMMAND_HANDLER_EX(IDC_TITLEFORMAT, EN_CHANGE, OnEditChange)
	END_MSG_MAP()
private:
	BOOL OnInitDialog(CWindow, LPARAM);
	void OnEditChange(UINT, int, CWindow);
	bool HasChanged();
	void OnChanged();

	const preferences_page_callback::ptr m_callback;
};
BOOL CMyPreferences::OnInitDialog(CWindow, LPARAM) {
	stringcvt::string_wide_from_utf8 wbuffer(cfg_format);
	SetDlgItemText(IDC_TITLEFORMAT,wbuffer);
	CheckDlgButton(IDC_ENABLED, cfg_enabled ? BST_CHECKED : BST_UNCHECKED);
	return FALSE;
}
void CMyPreferences::OnEditChange(UINT, int, CWindow) {
	OnChanged();
}
t_uint32 CMyPreferences::get_state() {
	t_uint32 state = preferences_state::resettable;
	if (HasChanged()) state |= preferences_state::changed;
	return state;
}
void CMyPreferences::reset() {
	cfg_enabled = true;
	CheckDlgButton(IDC_ENABLED,BST_CHECKED);
	cfg_format = "%title% - %album%";
	stringcvt::string_wide_from_utf8 wbuffer(cfg_format);
	SetDlgItemText(IDC_TITLEFORMAT,wbuffer);
	OnChanged();
}
void CMyPreferences::apply() {
	CString buf;
	cfg_enabled = (IsDlgButtonChecked(IDC_ENABLED) == BST_CHECKED);
	GetDlgItemText(IDC_TITLEFORMAT,buf);
	stringcvt::string_utf8_from_wide title(buf);
	cfg_format = title;
	OnChanged(); //our dialog content has not changed but the flags have - our currently shown values now match the settings so the apply button can be disabled
}
bool CMyPreferences::HasChanged() {
	CString buf;
	GetDlgItemText(IDC_TITLEFORMAT,buf);
	stringcvt::string_utf8_from_wide title(buf);
	return (IsDlgButtonChecked(IDC_ENABLED) == BST_CHECKED) != cfg_enabled || title != cfg_format;
}
void CMyPreferences::OnChanged() {
	//tell the host that our state has changed to enable/disable the apply button appropriately.
	m_callback->on_state_changed();
}
class preferences_page_myimpl : public preferences_page_impl<CMyPreferences> {
	// preferences_page_impl<> helper deals with instantiation of our dialog; inherits from preferences_page_v3.
public:
	const char * get_name() {return "mIRC Now Playing";}
	GUID get_guid() {
		// This is our GUID. Replace with your own when reusing the code.
		static const GUID guid = 
		{ 0x439c0c97, 0x84be, 0x4e39, { 0x96, 0x85, 0x55, 0xb4, 0xcc, 0x69, 0x47, 0x8b } };
		return guid;
	}
	GUID get_parent_guid() {return guid_display;}
};
static preferences_page_factory_t<preferences_page_myimpl> g_preferences_page_myimpl_factory;

/*
BOOL mircSendAll()
{
	HWND mirclient = FindWindowEx(0, 0, L"mIRC", NULL);

	if (mirclient)
	{
		HWND mdiclient, active;

		console::error("found mirc window");
		mdiclient = FindWindowEx( mirclient, 0, L"MDIClient", NULL);
		active = GetWindow(mdiclient, GW_CHILD);
		SendMessage(active, WM_USER + 200, 5, 0L);
		return TRUE;
	}
	return FALSE;
}*/

BOOL CALLBACK mircSendAll(HWND hWnd, LPARAM lParam)
{
	TCHAR wndclass[256];
	int do_status = 0;

	if (!RealGetWindowClass(hWnd, (TCHAR *)&wndclass, sizeof(wndclass))) return TRUE;

	if (stricmp((const char*)wndclass, (const char*)"mIRC")) return TRUE; 
	{
		HWND mdiclient, active;

		console::error("found mirc window");
		mdiclient = FindWindowEx(hWnd, 0, L"MDIClient", NULL);
		// channel = FindWindowEx(mdiclient, 0, "channel", NULL);
		active = GetWindow(mdiclient, GW_CHILD);
		SendMessage(active, WM_USER + 200, 5, 0L);
	}
	return 0;
}

void show_mirc_title(metadb_handle_ptr track)
{
	service_ptr_t<titleformat_object> format;
	static_api_ptr_t<titleformat_compiler>()->compile(format,cfg_format);
	string8 value;
	string8 title;
	track->format_title(NULL, value, format, NULL);
	title = "/me is playing: ";
	title += value;
	console::error(value);

	if (cfg_enabled)
	{
	HANDLE hFileMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, 4096, L"mIRC");
	if (hFileMap)
	{
		char *mData;
		mData = (char *)MapViewOfFile(hFileMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
		if (mData)
		{
			strcpy(mData, title.get_ptr());
			UnmapViewOfFile((LPCVOID)mData);
			EnumDesktopWindows(NULL,mircSendAll, 0);
			console::error("Successful");
		}
		
		CloseHandle(hFileMap);
	}
	}

}


class play_callback_mirc : public play_callback_static
{
	virtual void on_playback_starting(play_control::t_track_command p_command, bool p_paused) {}
	virtual void on_playback_new_track(metadb_handle_ptr p_track)
	{
		show_mirc_title(p_track);
	}
	virtual void on_playback_stop(play_control::t_stop_reason p_reason){}
	virtual void on_playback_seek(double p_time) {}
	virtual void on_playback_pause(bool p_state){}
	virtual void on_playback_edited(metadb_handle_ptr p_track) {}
	virtual void on_playback_dynamic_info(const file_info & info) {}
	virtual void on_playback_dynamic_info_track(const file_info & info) {}
	virtual void on_playback_time(double p_time) {}
	virtual void on_volume_change(float p_new_val) {};
	
	virtual unsigned get_flags() 
	{
		return flag_on_playback_new_track;
	}

};
static play_callback_static_factory_t<play_callback_mirc> mirc_callback_factory;