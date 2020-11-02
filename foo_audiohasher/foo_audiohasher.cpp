#include "targetver.h"
#include <Windows.h>
#include "../ATLHelpers/ATLHelpers.h"
#include "../shared/shared.h"
#include <atlframe.h>
#include "../helpers/helpers.h"

#include "resource.h"
#include "hash/sha1.h"

using namespace pfc;
// {3F15B59B-6729-454B-8537-6A8257D0FCF5}
static const GUID guid = 
{ 0x3f15b59b, 0x6729, 0x454b, { 0x85, 0x37, 0x6a, 0x82, 0x57, 0xd0, 0xfc, 0xf5 } };
// {5342A231-84B9-4ACC-8C4B-610E7BCA1331}
static const GUID guid_mygroup = 
{ 0x5342a231, 0x84b9, 0x4acc, { 0x8c, 0x4b, 0x61, 0xe, 0x7b, 0xca, 0x13, 0x31 } };

static contextmenu_group_popup_factory g_mygroup(guid_mygroup, contextmenu_groups::root, "Audio data hasher", 0);
static const int thread_priority_levels[7] = { THREAD_PRIORITY_IDLE, THREAD_PRIORITY_LOWEST, THREAD_PRIORITY_BELOW_NORMAL, THREAD_PRIORITY_NORMAL, THREAD_PRIORITY_ABOVE_NORMAL, THREAD_PRIORITY_HIGHEST, THREAD_PRIORITY_TIME_CRITICAL };


// {F7467D64-66F5-41A7-AF68-24A05D8B9DDD}
static const GUID guid_hasher_branch = 
{ 0xf7467d64, 0x66f5, 0x41a7, { 0xaf, 0x68, 0x24, 0xa0, 0x5d, 0x8b, 0x9d, 0xdd } };
// {7A9AFF78-5081-4E0E-A53A-CCE93E32F4D8}
static const GUID guid_cfg_thread_priority = 
{ 0x7a9aff78, 0x5081, 0x4e0e, { 0xa5, 0x3a, 0xcc, 0xe9, 0x3e, 0x32, 0xf4, 0xd8 } };
// {123644E6-68EE-42AE-89D6-5FA4DC223B10}
static const GUID guid_hash_branch = 
{ 0x123644e6, 0x68ee, 0x42ae, { 0x89, 0xd6, 0x5f, 0xa4, 0xdc, 0x22, 0x3b, 0x10 } };

// {A1356F12-A976-4866-8A9D-A095CF4457AF}
static const GUID guid_sha1 = 
{ 0xa1356f12, 0xa976, 0x4866, { 0x8a, 0x9d, 0xa0, 0x95, 0xcf, 0x44, 0x57, 0xaf } };
// {93CB93D5-36EB-4621-8664-1B8A2FB618EC}
static const GUID guid_md2 = 
{ 0x93cb93d5, 0x36eb, 0x4621, { 0x86, 0x64, 0x1b, 0x8a, 0x2f, 0xb6, 0x18, 0xec } };
// {5D95C890-4E96-4DBD-8770-A7182C427E92}
static const GUID guid_md4 = 
{ 0x5d95c890, 0x4e96, 0x4dbd, { 0x87, 0x70, 0xa7, 0x18, 0x2c, 0x42, 0x7e, 0x92 } };
// {DFFE896F-A6AB-4774-8F13-799807F8D67C}
static const GUID guid_md5 = 
{ 0xdffe896f, 0xa6ab, 0x4774, { 0x8f, 0x13, 0x79, 0x98, 0x7, 0xf8, 0xd6, 0x7c } };
// {DDF93F02-9501-49B2-940C-C7C6943C8AB9}
static const GUID guid_rmd128 = 
{ 0xddf93f02, 0x9501, 0x49b2, { 0x94, 0xc, 0xc7, 0xc6, 0x94, 0x3c, 0x8a, 0xb9 } };
// {396562F0-398B-4080-A00C-9D73C0A6E77F}
static const GUID guid_rmd160 = 
{ 0x396562f0, 0x398b, 0x4080, { 0xa0, 0xc, 0x9d, 0x73, 0xc0, 0xa6, 0xe7, 0x7f } };
// {FE32FDAA-DA44-4E78-BDC2-2C8FCF90BB49}
static const GUID guid_rmd256 = 
{ 0xfe32fdaa, 0xda44, 0x4e78, { 0xbd, 0xc2, 0x2c, 0x8f, 0xcf, 0x90, 0xbb, 0x49 } };
// {A45A80AF-3FD1-41F6-9FB3-35D91A0A0BD5}
static const GUID guid_rmd320 = 
{ 0xa45a80af, 0x3fd1, 0x41f6, { 0x9f, 0xb3, 0x35, 0xd9, 0x1a, 0xa, 0xb, 0xd5 } };
// {6A70036A-A577-4C92-8827-F01146D28767}
static const GUID guid_sha224 = 
{ 0x6a70036a, 0xa577, 0x4c92, { 0x88, 0x27, 0xf0, 0x11, 0x46, 0xd2, 0x87, 0x67 } };
// {36A74535-2A45-475A-AE0F-7C2DDFD08729}
static const GUID guid_sha256 = 
{ 0x36a74535, 0x2a45, 0x475a, { 0xae, 0xf, 0x7c, 0x2d, 0xdf, 0xd0, 0x87, 0x29 } };
// {2C922017-7081-472F-BEFD-8263563CCE87}
static const GUID guid_sha384 = 
{ 0x2c922017, 0x7081, 0x472f, { 0xbe, 0xfd, 0x82, 0x63, 0x56, 0x3c, 0xce, 0x87 } };
// {54234DF1-E8A4-4222-BFDC-608B71B1B619}
static const GUID guid_sha512 = 
{ 0x54234df1, 0xe8a4, 0x4222, { 0xbf, 0xdc, 0x60, 0x8b, 0x71, 0xb1, 0xb6, 0x19 } };
// {C7CF5E3F-125E-49BC-8CA6-048DEADAECFF}
static const GUID guid_tiger = 
{ 0xc7cf5e3f, 0x125e, 0x49bc, { 0x8c, 0xa6, 0x4, 0x8d, 0xea, 0xda, 0xec, 0xff } };
// {81D7BF9D-3DA7-4470-B80C-D7FCA8ADA462}
static const GUID guid_whirlpool = 
{ 0x81d7bf9d, 0x3da7, 0x4470, { 0xb8, 0xc, 0xd7, 0xfc, 0xa8, 0xad, 0xa4, 0x62 } };








static advconfig_integer_factory cfg_thread_priority("Thread priority (1-7)", guid_cfg_thread_priority, guid_hasher_branch, 0, 2, 1, 7 );
static advconfig_branch_factory hasher_tools_branch("Audio data hasher", guid_hasher_branch, advconfig_branch::guid_branch_tools, 0);

static advconfig_branch_factory hash_type_branch("Hash type", guid_hash_branch, guid_hasher_branch, 0);
static advconfig_checkbox_factory_t<true> cfg_sha1("SHA-1", guid_sha1, guid_hash_branch, 0, true);
static advconfig_checkbox_factory_t<true> cfg_md2("MD2", guid_md2, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_md4("MD4", guid_md4, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_md5("MD5", guid_md5, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_rmd128("RIPEMD-128", guid_rmd128, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_rmd160("RIPEMD-160", guid_rmd160, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_rmd256("RIPEMD-256", guid_rmd256, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_rmd320("RIPEMD-320", guid_rmd320, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_sha224("SHA2-224", guid_sha224, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_sha256("SHA2-256", guid_sha256, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_sha384("SHA2-384", guid_sha384, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_sha512("SHA2-512", guid_sha512, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_tiger("TIGER", guid_tiger, guid_hash_branch, 0, false);
static advconfig_checkbox_factory_t<true> cfg_whirlpool("WHIRLPOOL", guid_whirlpool, guid_hash_branch, 0, false);


VALIDATE_COMPONENT_FILENAME("foo_audiohasher.dll");
DECLARE_COMPONENT_VERSION(
	"Audio Hasher",
	"0.2",
	"A audio hashing/checksum component for foobar2000 1.1 ->\n"
	"Copyright (C) 2014 Brad Miller\n"
	"http://mudlord.info\n"
);

struct hasher_result
{
	enum status
	{
		error = 0,
		track
	};

	status                     m_status;
	pfc::string8               m_error_message;
	metadb_handle_list         m_handles;
	pfc::string8               m_hashes;

	hasher_result(status p_status)
		: m_status(p_status){ }

	hasher_result(status p_status, const metadb_handle_list & p_handles)
		: m_status(p_status), m_handles(p_handles) { }

	hasher_result(const hasher_result * in)
	{
		m_status        = in->m_status;
		m_hashes        = in->m_hashes;
		m_error_message = in->m_error_message;
		m_handles       = in->m_handles;
	}
};

double hash_track(pfc::string & sha1_hash,metadb_handle_ptr p_handle, double & p_progress, unsigned track_total, threaded_process_status & p_status, critical_section & p_critsec, abort_callback & p_abort )
{
	const t_uint32 decode_flags = input_flag_no_seeking | input_flag_no_looping; // tell the decoders that we won't seek and that we don't want looping on formats that support looping.
	input_helper m_decoder;
	file_info_impl m_info;
	service_ptr_t<file>        m_file, m_file_cached;
	sha1_context ctx;
	audio_chunk_impl_temporary m_chunk;
	double duration = 0, length = 0;

	try
	{
		filesystem::g_open_precache( m_file, p_handle->get_path(), p_abort );
		file_cached::g_create( m_file_cached, m_file, p_abort, 4 * 1024 * 1024 );
		m_file = m_file_cached;
	}
	catch (...)
	{
		m_file.release();
	}

	memset( &ctx, 0, sizeof( sha1_context ) );
	sha1_starts( &ctx );

	m_decoder.open( m_file, p_handle, input_flag_simpledecode, p_abort, false, true );
	m_decoder.get_info( p_handle->get_subsong_index(), m_info, p_abort );
	length = m_info.get_length();

	while ( m_decoder.run( m_chunk, p_abort ) )
	{
		p_abort.check();
		sha1_update(&ctx,(unsigned char*)m_chunk.get_data(),m_chunk.get_data_length());
		duration += m_chunk.get_duration();

		if ( length )
		{
			insync(p_critsec);
			p_progress += m_chunk.get_duration() / length / double(track_total);
			p_status.set_progress_float( p_progress );
		}
	}

	unsigned char sha1sum[20] ={0};
	sha1_finish(&ctx,sha1sum);
	pfc::string_formatter msg;
	for(int i = 0; i < 20; i++ )msg <<pfc::format_hex(sha1sum[i]);
    sha1_hash=msg;
	{
		insync(p_critsec);
		p_progress += ( length ? ( ( length - duration ) / length ) : 1.0 ) / double(track_total);
		p_status.set_progress_float( p_progress );
	}
	return duration;

}

static void RunHashResultsPopup( pfc::ptr_list_t<hasher_result> & m_results, double sample_duration, unsigned __int64 processing_duration, HWND p_parent );

class hasher_thread : public threaded_process_callback
{
	critical_section lock_status;
	threaded_process_status * status_callback;
	double m_progress;
	unsigned thread_count;
	abort_callback * m_abort;
	pfc::array_t<HANDLE> m_extra_threads;

	struct job
	{
		metadb_handle_list             m_handles;
		pfc::ptr_list_t<const char>    m_names;

		// Album mode stuff
		critical_section               m_lock;
		pfc::array_t<bool>             m_thread_in_use;
		size_t                         m_tracks_left;
		size_t                         m_current_thread;
		metadb_handle_list             m_handles_done;
		hasher_result          * m_scanner_result;

		job( bool p_is_album, const metadb_handle_list & p_handles )
		{
			m_handles = p_handles;
			m_tracks_left = p_handles.get_count();
			m_current_thread = 0;
			m_scanner_result = NULL;
		}

		~job()
		{
			m_names.delete_all();
			delete m_scanner_result;
		}
	};

		LONG input_items_total;
		volatile LONG input_items_remaining;

		critical_section lock_input_job_list;
		pfc::ptr_list_t<job> input_job_list;

		critical_section lock_input_name_list;
		pfc::ptr_list_t<const char> input_name_list;

		critical_section lock_output_list;
		pfc::ptr_list_t<hasher_result> output_list;
		double output_duration;

		FILETIME start_time, end_time;

		void report_error( const char * message, metadb_handle_ptr track )
	{
		hasher_result * result = NULL;
		try
		{
			result = new hasher_result( hasher_result::error );
			result->m_error_message = message;
			result->m_handles.add_item( track );
			{
				insync( lock_output_list );
				output_list.add_item( result );
			}
		}
		catch (...)
		{
			delete result;
		}
	}

		void report_error( const char * message, metadb_handle_list tracks )
		{
			hasher_result * result = NULL;
			try
			{
				result = new hasher_result( hasher_result::error );
				result->m_error_message = message;
				result->m_handles = tracks;
				{
					insync( lock_output_list );
					output_list.add_item( result );
				}
			}
			catch (...)
			{
				delete result;
			}
		}


	void scanner_process()
	{
		for (;;)
		{
			job * m_current_job = NULL;
			m_abort->check();
			{
				insync( lock_input_job_list );
				if ( ! input_job_list.get_count() ) break;
				m_current_job = input_job_list[ 0 ];
				input_job_list.remove_by_idx( 0 );
			}
			try
			{
			    {
					insync(lock_input_name_list);
					input_name_list.add_item( m_current_job->m_names[ 0 ] );
				}
				update_status();
				pfc::string hashed_res;
				double duration = hash_track( hashed_res, m_current_job->m_handles[ 0 ], m_progress, input_items_total, *status_callback, lock_status, *m_abort );
				hasher_result * m_current_result = new hasher_result(hasher_result::track, m_current_job->m_handles);
				m_current_result->m_hashes = hashed_res.get_ptr();
				{
					insync(lock_output_list);
					output_list.add_item( m_current_result );
					output_duration += duration;
				}
				{
					insync(lock_input_name_list);
					input_name_list.remove_item( m_current_job->m_names[ 0 ] );
				}
				InterlockedDecrement( &input_items_remaining );
				update_status();
			}
			catch (exception_aborted & e)
			{
				if ( m_current_job )
				{
					report_error( e.what(), m_current_job->m_handles );

					{
						insync(lock_input_name_list);
						for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
						{
							input_name_list.remove_item( m_current_job->m_names[ i ] );
						}
					}
				}
				delete m_current_job;
				break;
			}
			catch (...)
			{
				if ( m_current_job )
				{
					insync(lock_input_name_list);
					for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
					{
						input_name_list.remove_item( m_current_job->m_names[ i ] );
					}
				}
				delete m_current_job;
				throw;
			}
			if ( m_current_job )
			{
				insync(lock_input_name_list);
				for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
				{
					input_name_list.remove_item( m_current_job->m_names[ i ] );
				}
			}
			delete m_current_job; m_current_job = NULL;
		}

	}

	void update_status()
	{
		pfc::string8 title, paths;

		{
			insync( lock_input_name_list );

			for ( unsigned i = 0; i < input_name_list.get_count(); i++ )
			{
				const char * temp = input_name_list[ i ];
				if ( paths.length() ) paths += "; ";
				paths.add_string( temp );
			}
		}

		title = "Audio Hasher Progress";
		title += " (";
		title += pfc::format_int( input_items_total - input_items_remaining );
		title += "/";
		title += pfc::format_int( input_items_total );
		title += ")";

		{
			insync( lock_status );
			status_callback->set_title( title );
			status_callback->set_item( paths );
			status_callback->set_progress_float( m_progress );
		}
	}

	static DWORD CALLBACK g_entry(void* p_instance)
	{
		try
		{
			reinterpret_cast<hasher_thread*>(p_instance)->scanner_process();
		}
		catch (...) { }
		return 0;
	}

	void threads_start( unsigned count )
	{
		int priority = GetThreadPriority( GetCurrentThread() );

		for ( unsigned i = 0; i < count; i++ )
		{
			HANDLE thread = CreateThread( NULL, 0, g_entry, reinterpret_cast<void*>(this), CREATE_SUSPENDED, NULL );
			if ( thread != NULL )
			{
				SetThreadPriority( thread, priority );
				m_extra_threads.append_single( thread );
			}
		}

		for ( unsigned i = 0; i < m_extra_threads.get_count(); i++ )
		{
			ResumeThread( m_extra_threads[ i ] );
		}
	}

	void threads_stop()
	{
		for ( unsigned i = 0; i < m_extra_threads.get_count(); i++ )
		{
			HANDLE thread = m_extra_threads[ i ];
			WaitForSingleObject( thread, INFINITE );
			CloseHandle( thread );
		}

		m_extra_threads.set_count( 0 );
	}

public:
	hasher_thread()
	{
		input_items_remaining = input_items_total = 0;
		output_duration = 0;
		m_progress = 0;
	}

	void add_job_track( const metadb_handle_ptr & p_input )
	{
		metadb_handle_list p_list; p_list.add_item( p_input );
		job * m_job = new job( false, p_list );
		const char * m_path = p_input->get_path();
		t_size filename = pfc::scan_filename( m_path );
		t_size length = strlen( m_path + filename );
		char * m_name = new char[length + 1];
		strcpy_s(m_name, length + 1, m_path + filename);
		m_job->m_names.add_item( m_name );
		input_job_list.add_item( m_job );
		input_items_remaining = input_items_total += 1;
	}

	~hasher_thread()
	{
		input_job_list.delete_all();
		output_list.delete_all();
	}

	virtual void run(threaded_process_status & p_status,abort_callback & p_abort)
	{
		status_callback = &p_status;
		m_abort = &p_abort;

		update_status();

		//SetThreadPriority( GetCurrentThread(), thread_priority_levels[ cfg_thread_priority.get() - 1 ] );

		GetSystemTimeAsFileTime( &start_time );

#if defined(NDEBUG) || 1
		thread_count = 0;

		for ( unsigned i = 0; i < input_job_list.get_count(); i++ ) thread_count += input_job_list[ i ]->m_handles.get_count();

		thread_count = pfc::getOptimalWorkerThreadCountEx( min( thread_count, 4 ) );

		if ( thread_count > 1 ) threads_start( thread_count - 1 );
#else
		thread_count = 1;
#endif

		try
		{
			scanner_process();
		}
		catch (...) { }

		threads_stop();

		GetSystemTimeAsFileTime( &end_time );
	}

	virtual void on_done( HWND p_wnd, bool p_was_aborted )
	{
		threads_stop();

		if ( !p_was_aborted )
		{
			DWORD high = end_time.dwHighDateTime - start_time.dwHighDateTime;
			DWORD low = end_time.dwLowDateTime - start_time.dwLowDateTime;

			if ( end_time.dwLowDateTime < start_time.dwLowDateTime ) high--;

			unsigned __int64 timestamp = ((unsigned __int64)(high) << 32) + low;

			RunHashResultsPopup( output_list, output_duration, timestamp, core_api::get_main_window() );
		}
	}

};

class CMyResultsPopup : public CDialogImpl<CMyResultsPopup>, public CDialogResize<CMyResultsPopup>
{
public:
	CMyResultsPopup( const pfc::ptr_list_t<hasher_result> & initData, double sample_duration, unsigned __int64 processing_duration )
		: m_sample_duration( sample_duration ), m_processing_duration( processing_duration )
	{
		for ( unsigned i = 0; i < initData.get_count(); i++ )
		{
			hasher_result * result = new hasher_result( initData[ i ] );
			m_initData.add_item( result );
		}		
	}

	~CMyResultsPopup()
	{
		m_initData.delete_all();
	}

	enum { IDD = IDD_RESULTS };

	BEGIN_MSG_MAP( CMyResultsPopup )
		MSG_WM_INITDIALOG( OnInitDialog )
		COMMAND_HANDLER_EX( IDCANCEL, BN_CLICKED, OnCancel )
		MSG_WM_NOTIFY( OnNotify )
		CHAIN_MSG_MAP(CDialogResize<CMyResultsPopup>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP( CMyResultsPopup )
		DLGRESIZE_CONTROL( IDC_LISTVIEW, DLSZ_SIZE_X | DLSZ_SIZE_Y )
		DLGRESIZE_CONTROL( IDC_STATUS, DLSZ_SIZE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
	END_DLGRESIZE_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{
		DlgResize_Init();

		double processing_duration = (double)(m_processing_duration) * 0.0000001;
		double processing_ratio = m_sample_duration / processing_duration;

		pfc::string8_fast temp;

		temp = "Calculated in: ";
		temp += pfc::format_time_ex( processing_duration );
		temp += ", speed: ";
		temp += pfc::format_float( processing_ratio, 0, 2 );
		temp += "x";

		uSetDlgItemText( m_hWnd, IDC_STATUS, temp );

		m_listview = GetDlgItem( IDC_LISTVIEW );

		LVCOLUMN lvc = { 0 };
		lvc.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvc.fmt = LVCFMT_LEFT;
		lvc.pszText = _T("Name");
		lvc.cx = 225;
		lvc.iSubItem = 0;
		m_listview.InsertColumn( 0, &lvc );
		lvc.pszText = _T("Status");
		lvc.cx = 50;
		lvc.iSubItem = 1;
		m_listview.InsertColumn( 1, &lvc );
		lvc.fmt = LVCFMT_RIGHT;
		lvc.pszText = _T("Audio hash");
		lvc.cx = 230;
		m_listview.InsertColumn( 2, &lvc );

		for ( unsigned i = 0, index = 0; i < m_initData.get_count(); i++ )
		{
			for ( unsigned j = 0; j < m_initData[ i ]->m_handles.get_count(); j++, index++ )
			{
				m_listview.InsertItem( index, LPSTR_TEXTCALLBACK );
				m_listview.SetItemText( index, 1, LPSTR_TEXTCALLBACK );
				m_listview.SetItemText( index, 2, LPSTR_TEXTCALLBACK );
			}
		}

		if ( !static_api_ptr_t<titleformat_compiler>()->compile( m_script, "%title%" ) )
			m_script.release();

		ShowWindow(SW_SHOW);

		unsigned error_tracks = 0;
		unsigned total_tracks = 0;

		for ( unsigned i = 0; i < m_initData.get_count(); i++ )
		{
			unsigned count = m_initData[ i ]->m_handles.get_count();
			if ( m_initData[ i ]->m_status == hasher_result::error )
			{
				error_tracks += count;
			}
			total_tracks += count;
		}

		if ( error_tracks )
		{
			if ( error_tracks == total_tracks ) GetDlgItem( IDOK ).EnableWindow( FALSE );

			popup_message::g_show( pfc::string_formatter() << pfc::format_int( error_tracks ) << " out of " << pfc::format_int( total_tracks ) << " items could not be processed.", "EBU R128 Gain Scan - warning", popup_message::icon_error );
		}

		return TRUE;
	}

	void get_offsets( unsigned item, unsigned & job_number, unsigned & item_number )
	{
		for ( unsigned i = 0; i < m_initData.get_count(); i++ )
		{
			hasher_result * result = m_initData[ i ];
			if ( item >= result->m_handles.get_count() ) item -= result->m_handles.get_count();
			else
			{
				job_number = i;
				item_number = item;
				return;
			}
		}
		job_number = 0;
		item_number = 0;
	}

	LRESULT OnNotify( int, LPNMHDR message )
	{
		if ( message->hwndFrom == m_listview.m_hWnd )
		{
			switch ( message->code )
			{
			case LVN_GETDISPINFO:
				{
					LV_DISPINFO *pLvdi = (LV_DISPINFO *)message;

					unsigned job_number = 0, item_number = 0;

					get_offsets( pLvdi->item.iItem, job_number, item_number );
					hasher_result * result = m_initData[ job_number ];

					switch (pLvdi->item.iSubItem)
					{
					case 0:
						if ( m_script.is_valid() ) result->m_handles[ item_number ]->format_title( NULL, m_temp, m_script, NULL );
						else m_temp.reset();
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 1:
						if ( result->m_status != hasher_result::error ) m_temp = "Success";
						else m_temp = result->m_error_message;
						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;

					case 2:
						if ( result->m_status != hasher_result::error )
						{
							m_temp = result->m_hashes;
						}
						else m_temp.reset();

						m_convert.convert( m_temp );
						pLvdi->item.pszText = (TCHAR *) m_convert.get_ptr();
						break;
					}
				}
				break;
			}
		}

		return 0;
	}
	void OnCancel( UINT, int id, CWindow )
	{
		DestroyWindow();
	}

	double m_sample_duration;
	unsigned __int64 m_processing_duration;

	pfc::ptr_list_t<hasher_result> m_initData;

	CListViewCtrl m_listview;
	service_ptr_t<titleformat_object> m_script;
	pfc::string8_fast m_temp;
	pfc::stringcvt::string_os_from_utf8_fast m_convert;
};

static void RunHashResultsPopup( pfc::ptr_list_t<hasher_result> & p_data, double sample_duration, unsigned __int64 processing_duration, HWND p_parent )
{
	CMyResultsPopup * popup = new CWindowAutoLifetime<ImplementModelessTracking<CMyResultsPopup>>( p_parent, p_data, sample_duration, processing_duration );
}



class calculate_hash : public threaded_process_callback {
public:
	calculate_hash(metadb_handle_list_cref items) : m_items(items), m_peak() {
	}

	void on_init(HWND p_wnd) {}
	void run(threaded_process_status & p_status,abort_callback & p_abort) {
		try {
			const t_uint32 decode_flags = input_flag_no_seeking | input_flag_no_looping; // tell the decoders that we won't seek and that we don't want looping on formats that support looping.
			input_helper input;
			audio_hash.set_count(m_items.get_size());
			for(t_size walk = 0; walk < m_items.get_size(); ++walk) {

				p_abort.check(); // in case the input we're working with fails at doing this
				p_status.set_progress(walk, m_items.get_size());
				p_status.set_progress_secondary(0);
				p_status.set_item_path( m_items[walk]->get_path() );
				input.open(NULL, m_items[walk], decode_flags, p_abort);
				
				double length;
				{ // fetch the track length for proper dual progress display;
					file_info_impl info;
					// input.open should have preloaded relevant info, no need to query the input itself again.
					// Regular get_info() may not retrieve freshly loaded info yet at this point (it will start giving the new info when relevant info change callbacks are dispatched); we need to use get_info_async.
					if (m_items[walk]->get_info_async(info)) length = info.get_length();
					else length = 0;
				}

				memset( &ctx, 0, sizeof( sha1_context ) );
				sha1_starts( &ctx );
				audio_chunk_impl_temporary l_chunk;
				
				double decoded = 0;
				while(input.run(l_chunk, p_abort)) { // main decode loop
				    sha1_update(&ctx,(unsigned char*)l_chunk.get_data(),l_chunk.get_data_length());
					//m_peak = l_chunk.get_peak(m_peak);
					if (length > 0) { // don't bother for unknown length tracks
						decoded += l_chunk.get_duration();
						if (decoded > length) decoded = length;
						p_status.set_progress_secondary_float(decoded / length);
					}
					p_abort.check(); // in case the input we're working with fails at doing this
				}
				unsigned char sha1sum[20] ={0};
				sha1_finish(&ctx,sha1sum);
				pfc::string_formatter msg;
			    for(int i = 0; i < 20; i++ )msg <<pfc::format_hex(sha1sum[i]);
				audio_hash[walk]=msg;
				
				



			}
		} catch(std::exception const & e) {
			m_failMsg = e.what();
		}
	}
	void on_done(HWND p_wnd,bool p_was_aborted) {
		if (!p_was_aborted) {
			if (!m_failMsg.is_empty()) {
				popup_message::g_complain("Peak scan failure", m_failMsg);
			} else {
				pfc::string_formatter result;

				
				for(t_size walk = 0; walk < m_items.get_size(); ++walk) {
				    result << "Filename:\n";
					result << m_items[walk] << "\n";
					result << "Hash: " << audio_hash[walk] << "\n";
				}
				popup_message::g_show(result,"Hashing results");
			}
		}
	}
private:
	audio_sample m_peak;
	pfc::string8 m_failMsg;
	pfc::array_t<string8> audio_hash;
	sha1_context ctx;
	const metadb_handle_list m_items;
};

void RunCalculatePeak(metadb_handle_list_cref data) {
	try {
		if (data.get_count() == 0) throw pfc::exception_invalid_params();
		service_ptr_t<threaded_process_callback> cb = new service_impl_t<calculate_hash>(data);
		static_api_ptr_t<threaded_process>()->run_modeless(
			cb,
			threaded_process::flag_show_progress_dual | threaded_process::flag_show_item | threaded_process::flag_show_abort,
			core_api::get_main_window(),
			"Calculating audio data hashes...");
	} catch(std::exception const & e) {
		popup_message::g_complain("Could not start audio hashing process", e);
	}
}

class audiohasher : public contextmenu_item_simple {
public:
    GUID get_parent() {return guid_mygroup;}
	unsigned get_num_items() {return 1;}

	void get_item_name(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case 0: p_out = "Calculate audio data hash"; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}

	void context_command(unsigned p_index,metadb_handle_list_cref p_data,const GUID& p_caller) {
		metadb_handle_list input_files = p_data;
		input_files.remove_duplicates();
		service_ptr_t<hasher_thread> p_callback = new service_impl_t< hasher_thread >;
		switch(p_index) {
			case 0:
				
				for ( unsigned i = 0; i < input_files.get_count(); i++ )
				{
				p_callback->add_job_track( input_files[ i ] );
				}
				threaded_process::g_run_modeless( p_callback, threaded_process::flag_show_abort | threaded_process::flag_show_progress | threaded_process::flag_show_item | threaded_process::flag_show_delayed, core_api::get_main_window(), "Audio hasher status" );
				break;
			default:
				uBugCheck();
		}
	}

	GUID get_item_guid(unsigned p_index) {
		// These GUIDs identify our context menu items. Substitute with your own GUIDs when reusing code.
		static const GUID guid_test1 ={ 0xe87641bb, 0xbc5f, 0x468c, { 0x95, 0x1c, 0x8c, 0xa3, 0xf6, 0x8a, 0x36, 0xf1 } };
		switch(p_index) {
			case 0: return guid_test1;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}

	}
	bool get_item_description(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case 0:
				p_out = "Calculates audio data hashes on selected files.";
				return true;
			default:
				uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}

};

static contextmenu_item_factory_t<audiohasher> g_myitem_factory;
