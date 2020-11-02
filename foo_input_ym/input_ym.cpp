#define _WIN32_WINNT 0x0501
#include <foobar2000.h>

#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "StSoundGplPackage/StSoundLibrary/StSoundLibrary.h"

// {2E8591C7-E000-4297-8BCA-34679FC41116}
static const GUID guid_cfg_ym_parent = 
{ 0x2e8591c7, 0xe000, 0x4297, { 0x8b, 0xca, 0x34, 0x67, 0x9f, 0xc4, 0x11, 0x16 } };
// {528C8E94-25AA-44D2-B512-B36833E89788}
static const GUID guid_cfg_lowpass = 
{ 0x528c8e94, 0x25aa, 0x44d2, { 0xb5, 0x12, 0xb3, 0x68, 0x33, 0xe8, 0x97, 0x88 } };
// {203FB8C0-7867-4072-95EE-D2D8F46C617D}
static const GUID guid_cfg_loop = 
{ 0x203fb8c0, 0x7867, 0x4072, { 0x95, 0xee, 0xd2, 0xd8, 0xf4, 0x6c, 0x61, 0x7d } };

advconfig_branch_factory cfg_ym_parent("YM decoder", guid_cfg_ym_parent, advconfig_branch::guid_branch_playback, 0);
advconfig_checkbox_factory cfg_lowpass("Use lowpass filter", guid_cfg_lowpass, guid_cfg_ym_parent, 0, false);
advconfig_checkbox_factory cfg_loop("Loop indefinitely", guid_cfg_loop, guid_cfg_ym_parent, 0, false);

class input_ym
{
	YMMUSIC * m_player;
	ymMusicInfo_t m_info;
	t_filestats m_stats;
	long length;
	bool first_block,loop, use_lowpass;
	pfc::array_t< t_int16 > sample_buffer;
	pfc::array_t< t_uint8 > file_buffer;
public:
	input_ym()
	{
		m_player = ymMusicCreate();
	}

	~input_ym()
	{
		ymMusicDestroy(m_player);
	}

	void open( service_ptr_t<file> m_file, const char * p_path, t_input_open_reason p_reason, abort_callback & p_abort )
	{
		if ( p_reason == input_open_info_write ) throw exception_io_data();
		input_open_file_helper( m_file, p_path, p_reason, p_abort );

		m_stats = m_file->get_stats( p_abort );
		t_uint8            * ptr;
		unsigned             size;
		t_filesize size64 = m_file->get_size_ex( p_abort );
		if ( size64 > ( 1 << 24 ) )
			throw exception_io_data();
		size = (unsigned) size64;
		file_buffer.set_size( size );
		ptr = file_buffer.get_ptr();
		m_file->read_object( ptr, size, p_abort );

		bool yay = ymMusicLoadMemory(m_player,ptr,size);
		if ( !yay ) throw exception_io_data();
		ymMusicGetInfo(m_player,&m_info);
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		YMMUSIC *  m_info = ymMusicCreate();
		ymMusicLoadMemory(m_info,file_buffer.get_ptr(), file_buffer.get_size());
		ymMusicInfo_t info;
		ymMusicGetInfo(m_info,&info);

		p_info.info_set( "encoding", "synthesized" );
		p_info.info_set( "codec", "YM" );
		p_info.info_set_int( "channels", 1 );
		p_info.meta_set( "title", pfc::stringcvt::string_utf8_from_ansi( info.pSongName) );
		p_info.meta_set( "artist", pfc::stringcvt::string_utf8_from_ansi( info.pSongAuthor) );
		p_info.meta_set( "comment", pfc::stringcvt::string_utf8_from_ansi( info.pSongComment) );
		p_info.set_length( info.musicTimeInSec );

		ymMusicDestroy(m_info);
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		loop = cfg_loop;
		if ( p_flags & input_flag_no_looping) loop = false;
		bool lowpass = cfg_lowpass;
		ymMusicSetLoopMode(m_player,loop);
		ymMusicSetLowpassFiler(m_player,lowpass);
		ymMusicPlay(m_player);
		first_block = true;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if (!loop && m_info.musicTimeInMs == ymMusicGetPos(m_player)) return false;
		int nbSample = 500 / sizeof(ymsample);
		sample_buffer.grow_size( nbSample );
		ymMusicCompute(m_player,sample_buffer.get_ptr(), nbSample);
		p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), nbSample * 2, 44100, 1, 16, audio_chunk::channel_config_mono );
		return true;
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		long seek_ms = audio_math::time_to_samples( p_seconds, 1000 );
		ymMusicSeek(m_player,seek_ms);
		first_block = true;
	}

	bool decode_can_seek()
	{
		return true;
	}

	bool decode_get_dynamic_info( file_info & p_out, double & p_timestamp_delta )
	{
		if ( first_block )
		{
			first_block = false;
			p_out.info_set_int( "samplerate", 44100 );
			return true;
		}
		return false;
	}

	bool decode_get_dynamic_info_track( file_info & p_out, double & p_timestamp_delta )
	{
		return false;
	}

	void decode_on_idle( abort_callback & p_abort ) { }

	void retag( const file_info & p_info, abort_callback & p_abort )
	{
		throw exception_io_data();
	}

	static bool g_is_our_content_type( const char * p_content_type )
	{
		return false;
	}

	static bool g_is_our_path( const char * p_full_path, const char * p_extension )
	{
		return !stricmp( p_extension, "ym" );
	}
};

class ym_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 1;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		static const char * names[] = { "Atari ST files"};
		if (idx > 0) return false;
		out = names[ idx ];
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		static const char * extensions[] = { "YM"};
		out = "*.";
		if (idx > 0) return false;
		out += extensions[ idx ];
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};

static input_singletrack_factory_t< input_ym >             g_input_factory_ym;
static service_factory_single_t  <ym_file_types>           g_input_file_type_hvl_factory;

#define MYVERSION "0.3"

DECLARE_COMPONENT_VERSION(
    "YM Decoder",
	MYVERSION,
	pfc::stringcvt::string_utf8_from_os(L"A YM emulator for foobar2000 1.1 ->\nWritten by mudlord\nST-Sound emulator library by Arnaud Carré\nUsing StSoundLibrary 1.3"));
VALIDATE_COMPONENT_FILENAME("foo_input_ym.dll");
