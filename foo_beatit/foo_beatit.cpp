#include "targetver.h"

#include <Windows.h>
#include "../ATLHelpers/ATLHelpers.h"
#include "../shared/shared.h"
#include <atlframe.h>
#include "../helpers/helpers.h"

#include "resource.h"
#include "MiniBpm/MiniBpm.h"
#include "soundtouch/BPMDetect.h"
#include "fraganator/bpm_auto_analysis.h"
#include "bpm_manual_dialog.h"
#include "fraganator/globals.h"
#include "fraganator/format_bpm.h"

using namespace breakfastquay;

using namespace pfc;
using namespace soundtouch;

VALIDATE_COMPONENT_FILENAME("foo_beatit.dll");
DECLARE_COMPONENT_VERSION(
	"Beat It",
	"0.3",
	"A beat detection component for foobar2000 1.3 ->\n"
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
	double                    m_hashes;

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

static void RunHashResultsPopup( pfc::ptr_list_t<hasher_result> & m_results, unsigned __int64 processing_duration, HWND p_parent );

void get_track_beats(double & beats,metadb_handle_ptr p_handle, threaded_process_status & p_status, critical_section & p_critsec, abort_callback & p_abort )
{
	const t_uint32 decode_flags = input_flag_no_seeking | input_flag_no_looping; // tell the decoders that we won't seek and that we don't want looping on formats that support looping.
	input_helper m_decoder;
	file_info_impl m_info;
	service_ptr_t<file>        m_file, m_file_cached;
	
	audio_chunk_impl_temporary m_chunk;
	
	double length = 0;

   if(bpm_algorithm == 0)
   {
	   bpm_auto_analysis bpm(p_handle);
	   double bpm_result = bpm.run_safe(p_status, p_abort);
	   beats = bpm_result;
   }

   if (bpm_algorithm == 1)
   {
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

	m_decoder.open( m_file, p_handle, input_flag_simpledecode, p_abort, false, true );
	m_decoder.get_info( p_handle->get_subsong_index(), m_info, p_abort );
	length = m_info.get_length();

	m_decoder.run( m_chunk, p_abort );
	int m_ch = m_chunk.get_channels();
	int srate = m_chunk.get_srate();

	MiniBPM* bpm  = new MiniBPM(srate);
	bpm->setBPMRange(60,250);

	while ( m_decoder.run( m_chunk, p_abort ) )
	{
		p_abort.check();
		bpm->process(m_chunk.get_data(),m_chunk.get_sample_count());
	}
	pfc::string_formatter msg;
	beats = bpm->estimateTempo();
	delete bpm;
   }

   if (bpm_algorithm == 2)
   {
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
	   m_decoder.open( m_file, p_handle, input_flag_simpledecode, p_abort, false, true );
	   m_decoder.get_info( p_handle->get_subsong_index(), m_info, p_abort );
	   length = m_info.get_length();
	   m_decoder.run( m_chunk, p_abort );
	   int m_ch = m_chunk.get_channels();
	   int srate = m_chunk.get_srate();
	  BPMDetect* bpm  = new BPMDetect(m_ch,srate);
	   while ( m_decoder.run( m_chunk, p_abort ) )
	   {
		   p_abort.check();
		   bpm->inputSamples(m_chunk.get_data(),m_chunk.get_sample_count());
	   }
	  beats = bpm->getBpm();
	  delete bpm;
   }


}






class hasher_thread : public threaded_process_callback
{
	critical_section lock_status;
	threaded_process_status * status_callback;
	int m_progress;
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
				double beats;
				get_track_beats( beats, m_current_job->m_handles[ 0 ], *status_callback, lock_status, *m_abort );
				hasher_result * m_current_result = new hasher_result(hasher_result::track, m_current_job->m_handles);
				m_current_result->m_hashes = beats;
				{
					insync(lock_output_list);
					output_list.add_item( m_current_result );
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
			catch (exception_io & e)
			{
				if ( m_current_job )
				{
					report_error( e.what(), m_current_job->m_handles );

					{
						insync(lock_input_name_list);
						for ( unsigned i = 0; i < m_current_job->m_names.get_count(); i++ )
						{
							input_name_list.remove_item( m_current_job->m_names[ i ] );
							InterlockedDecrement( &input_items_remaining );
						}
					}

					{
						insync(lock_status);
						
					}

					update_status();
				}
			}

			catch (std::exception & e)
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

		title = "Beat Detection Progress";
		title += " (";
		title += pfc::format_int( input_items_total - input_items_remaining );
		title += "/";
		title += pfc::format_int( input_items_total );
		title += ")";

		{
			insync( lock_status );
			m_progress ++;
			status_callback->set_title( title );
			status_callback->set_item( paths );
			status_callback->set_progress( m_progress,input_items_total);

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

			RunHashResultsPopup( output_list, timestamp, core_api::get_main_window() );
		}
	}

};

class rg_apply_filter : public file_info_filter
{
	pfc::ptr_list_t<hasher_result> m_results;

	bool find_offsets( metadb_handle_ptr p_location, unsigned & result_offset, unsigned & item_offset )
	{
		for ( unsigned i = 0; i < m_results.get_count(); i++ )
		{
			hasher_result * result = m_results[ i ];
			for ( unsigned j = 0; j < result->m_handles.get_count(); j++ )
			{
				if ( result->m_handles[ j ] == p_location )
				{
					result_offset = i;
					item_offset = j;
					return true;
				}
			}
		}
		result_offset = 0;
		item_offset = 0;
		return false;
	}

public:
	rg_apply_filter( const pfc::ptr_list_t<hasher_result> & p_list )
	{
		for(t_size n = 0; n < p_list.get_count(); n++) {
			hasher_result * in_result = p_list[n];
			if ( in_result->m_status != hasher_result::error )
			{
				hasher_result * result = new hasher_result( in_result );
				m_results.add_item(result);
			}
		}
	}

	~rg_apply_filter()
	{
		m_results.delete_all();
	}

	virtual bool apply_filter(metadb_handle_ptr p_location,t_filestats p_stats,file_info & p_info)
	{
		unsigned result_offset, item_offset;
		if (find_offsets(p_location, result_offset, item_offset))
		{
			hasher_result * result = m_results[ result_offset ];
			format_bpm bpm_value(result->m_hashes);
			p_info.meta_set(bpm_config_bpm_tag, bpm_value);
			return true;
		}
		else
		{
			return false;
		}
	}
};

class CMyResultsPopup : public CDialogImpl<CMyResultsPopup>, public CDialogResize<CMyResultsPopup>
{
public:
	CMyResultsPopup( const pfc::ptr_list_t<hasher_result> & initData, unsigned __int64 processing_duration )
		:m_processing_duration( processing_duration )
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
		COMMAND_HANDLER_EX( IDUPDATE, BN_CLICKED, OnAccept )
		COMMAND_HANDLER_EX(IDDOUBLEBPM1, BN_CLICKED, OnDoubleBPMClicked)
		COMMAND_HANDLER_EX(IDHALVEBPM1, BN_CLICKED, OnHalveBPMClicked)
		NOTIFY_HANDLER_EX(IDC_LISTVIEW, LVN_ITEMCHANGED, OnItemChanged)
		MSG_WM_NOTIFY( OnNotify )
		CHAIN_MSG_MAP(CDialogResize<CMyResultsPopup>)
	END_MSG_MAP()

	BEGIN_DLGRESIZE_MAP( CMyResultsPopup )
		DLGRESIZE_CONTROL( IDC_LISTVIEW, DLSZ_SIZE_X | DLSZ_SIZE_Y )
		DLGRESIZE_CONTROL( IDC_STATUS, DLSZ_SIZE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL( IDUPDATE, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL(IDHALVEBPM1, DLSZ_MOVE_X | DLSZ_MOVE_Y )
		DLGRESIZE_CONTROL(IDDOUBLEBPM1, DLSZ_MOVE_X | DLSZ_MOVE_Y )
	END_DLGRESIZE_MAP()

private:
	BOOL OnInitDialog(CWindow, LPARAM)
	{
		DlgResize_Init();

		double processing_duration = (double)(m_processing_duration) * 0.0000001;

		pfc::string8_fast temp;

		temp = "Calculated in: ";
		temp += pfc::format_time_ex( processing_duration );
		

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
		lvc.pszText = _T("BPM");
		lvc.cx = 50;
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
		ListView_SetExtendedListViewStyle(m_listview.m_hWnd, LVS_EX_FULLROWSELECT );
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

			case NM_CLICK:
				{
					LONG iItem = ListView_GetNextItem(m_listview.m_hWnd, -1, LVNI_SELECTED);
					if (iItem != -1) 
					{ 
					}
				}

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
							pfc::string_formatter msg;
								format_bpm bpm_value(result->m_hashes);
							msg << bpm_value;
							m_temp = msg;
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

	LRESULT OnDoubleBPMClicked(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		ScaleSelectionBPM(2.0);
		return 0;
	}

	LRESULT OnHalveBPMClicked(UINT uNotifyCode, int nID, CWindow wndCtl)
	{
		ScaleSelectionBPM(0.5);
		return 0;
	}

	void EnableScaleBPMButtons()
	{
		CListViewCtrl listView(GetDlgItem(IDC_LISTVIEW));

		UINT selected = listView.GetSelectedCount();

		GetDlgItem(IDDOUBLEBPM1).EnableWindow(selected > 0);
		GetDlgItem(IDHALVEBPM1).EnableWindow(selected > 0);
	}

	LRESULT OnItemChanged(LPNMHDR pnmh)
	{
		EnableScaleBPMButtons();

		return 0;
	}


	void OnAccept( UINT, int id, CWindow )
	{
		metadb_handle_list list;
		for ( unsigned i = 0; i < m_initData.get_count(); i++ )
		{
			hasher_result * result = m_initData[ i ];
			if ( result->m_status != hasher_result::error )
			{
				list.add_items( result->m_handles );
			}
		}
		static_api_ptr_t<metadb_io_v2>()->update_info_async( list, new service_impl_t< rg_apply_filter >( m_initData ), core_api::get_main_window(), 0, 0 );

		DestroyWindow();
	}


	void ScaleSelectionBPM(double p_factor)
	{
		CWindow result_list = GetDlgItem(IDC_LISTVIEW);

		int listview_index = -1;
		metadb_handle_list list;
		pfc::ptr_list_t<hasher_result> data;

		while ((listview_index = ListView_GetNextItem(result_list, listview_index, LVIS_SELECTED)) != -1)
		{
			hasher_result * result = m_initData[ listview_index ];
			if ( result->m_status != hasher_result::error )
			{
				
				list.add_items( result->m_handles );
			}
			format_bpm bpm_value(result->m_hashes);
			listview_helper::set_item_text(result_list, listview_index, 1, bpm_value);
			hasher_result * result_add = new hasher_result(  result );
			data.add_item( result_add );
		}
		static_api_ptr_t<metadb_io_v2>()->update_info_async( list, new service_impl_t< rg_apply_filter >( data ), core_api::get_main_window(), 0, 0 );
		//DestroyWindow();
	}
	unsigned __int64 m_processing_duration;

	pfc::ptr_list_t<hasher_result> m_initData;

	CListViewCtrl m_listview;
	service_ptr_t<titleformat_object> m_script;
	pfc::string8_fast m_temp;
	pfc::stringcvt::string_os_from_utf8_fast m_convert;
};

static void RunHashResultsPopup( pfc::ptr_list_t<hasher_result> & p_data, unsigned __int64 processing_duration, HWND p_parent )
{
	CMyResultsPopup * popup = new CWindowAutoLifetime<ImplementModelessTracking<CMyResultsPopup>>( p_parent, p_data, processing_duration );
}









static const GUID guid = 
{ 0x13fee5fe, 0xdfdd, 0x4a2a, { 0xb1, 0xa4, 0xd5, 0x0, 0xf5, 0xa4, 0xa3, 0x43 } };
static contextmenu_group_popup_factory g_mygroup(guid, contextmenu_groups::root, "BPM detection", 0);

class audiohasher : public contextmenu_item_simple {
public:
	// {13FEE5FE-DFDD-4A2A-B1A4-D500F5A4A343}
	

    GUID get_parent() {return  guid;}
	unsigned get_num_items() {return 4;}

	void get_item_name(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case 0: p_out = "Calculate beats per minute"; break;
			case 1: p_out = "Manually calculate beats per minute"; break;
			case 2: p_out = "Double  BPM of selected tracks"; break;
			case 3: p_out = "Halve  BPM of selected tracks"; break;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}

	void do_manual()
	{
		bpm_manual_dialog* dlg = new bpm_manual_dialog();
		dlg->Create(core_api::get_main_window(), NULL);
		dlg->ShowWindow(SW_SHOWNORMAL);
	}

	void run_scale_bpm(metadb_handle_list_cref p_data, double p_scale)
	{
		const pfc::string8 bpm_tag = bpm_config_bpm_tag;

		metadb_handle_list tracks;
		pfc::list_t<file_info_impl> infos;

		tracks.prealloc(p_data.get_size());
		infos.prealloc(p_data.get_size());

		// For each item in the playlist selection
		for (t_size index = 0; index < p_data.get_size(); index++)
		{
			metadb_handle_ptr track = p_data[index];
			file_info_impl info;

			if (track->get_info(info) && info.meta_exists(bpm_tag))
			{
				tracks.add_item(track);
				infos.add_item(info);
			}
		}

		for (t_size index = 0; index < infos.get_size(); index++)
		{
			const char * str = infos[index].meta_get(bpm_tag, 0);

			float bpm = 0.0f;
			if (sscanf_s(str, "%f", &bpm) == 1)
			{
				bpm = static_cast<float>(bpm * p_scale);

				infos[index].meta_set(bpm_tag, format_bpm(bpm));
			}
		}

		static_api_ptr_t<metadb_io_v2>()->update_info_async_simple(tracks,
			pfc::ptr_list_const_array_t<const file_info, file_info_impl *>(infos.get_ptr(), infos.get_size()),
			core_api::get_main_window(),
			/* p_op_flags */ 0,
			/* p_notify */ NULL);
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
				threaded_process::g_run_modeless( p_callback, threaded_process::flag_show_abort | threaded_process::flag_show_item | threaded_process::flag_show_delayed, core_api::get_main_window(), "Audio hasher status" );
				break;
			case 1:
				do_manual();
				break;
			case 2:
				run_scale_bpm(p_data, 2.0);
				break;
			case 3:
				run_scale_bpm(p_data, 0.5);
				break;

			default:
				uBugCheck();
		}
	}

	GUID get_item_guid(unsigned p_index) {
		// These GUIDs identify our context menu items. Substitute with your own GUIDs when reusing code.
		// {4A11CBEA-120F-4F6B-9BC3-85B7DBB5F222}
		static const GUID guid_test1 = 
		{ 0x4a11cbea, 0x120f, 0x4f6b, { 0x9b, 0xc3, 0x85, 0xb7, 0xdb, 0xb5, 0xf2, 0x22 } };
		// {4C226F48-FEC5-487F-83CE-2A15B348A969}
		static const GUID guid2 = 
		{ 0x4c226f48, 0xfec5, 0x487f, { 0x83, 0xce, 0x2a, 0x15, 0xb3, 0x48, 0xa9, 0x69 } };
		// {7F709E18-F153-449C-90AD-486366AA2201}
		static const GUID guid3 = 
		{ 0x7f709e18, 0xf153, 0x449c, { 0x90, 0xad, 0x48, 0x63, 0x66, 0xaa, 0x22, 0x1 } };
		// {AFC583F6-EA9E-4F0C-A9C9-4FB75C634F9A}
		static const GUID guid4 = 
		{ 0xafc583f6, 0xea9e, 0x4f0c, { 0xa9, 0xc9, 0x4f, 0xb7, 0x5c, 0x63, 0x4f, 0x9a } };



		switch(p_index) {
			case 0: return guid_test1;
		    case 1: return guid2;
			case 2: return guid3;
			case 3: return guid4;
			default: uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}

	}
	bool get_item_description(unsigned p_index,pfc::string_base & p_out) {
		switch(p_index) {
			case 0:
				p_out = "Calculates beats per minute on tracks.";
				return true;
			case 1:
				p_out = "Calculates beats per minute on track manually.";
				return true;
			case 2: p_out = "Double  BPM of selected tracks"; return true;
			case 3: p_out = "Halve  BPM of selected tracks"; return true;
			default:
				uBugCheck(); // should never happen unless somebody called us with invalid parameters - bail
		}
	}

};

static contextmenu_item_factory_t<audiohasher> g_myitem_factory;
