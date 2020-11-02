#define _WIN32_WINNT 0x0501
#include <foobar2000.h>

#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"

#include "fclib/fc14audiodecoder.h"

static const char * extensions[]=
{
	"fc", "fc13", "fc14", "smod"
};

static bool g_test_extension(const char * ext)
{
	int n;
	for(n=0;n<tabsize(extensions);n++)
	{
		if (!stricmp(ext,extensions[n])) return true;
	}
	return false;
}

class input_fc
{
	void *fc_decoder;
	t_filestats m_stats;
	long length;
	bool first_block,loop;
	pfc::array_t< t_int16 > sample_buffer;
	pfc::array_t< t_uint8 > file_buffer;
public:
	input_fc()
	{
		fc_decoder = NULL;
	}

	~input_fc()
	{
		if (fc_decoder) fc14dec_delete(fc_decoder);
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
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		void *info = NULL;
		info = fc14dec_new();
		fc14dec_init(info,file_buffer.get_ptr(), file_buffer.get_size());
		fc14dec_mixer_init(info, 44100,16,2,0x0000);
		p_info.info_set( "encoding", "synthesized" );
		p_info.info_set( "codec", pfc::stringcvt::string_utf8_from_ansi(  fc14dec_format_name(info) ) );
		p_info.info_set_int( "channels", 2 );
		unsigned long ms = fc14dec_duration(info);
		ms /= 1000;
		p_info.set_length(ms);
		if (info) fc14dec_delete(info);
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{     
		if (fc_decoder) fc14dec_delete(fc_decoder);
		fc_decoder = fc14dec_new();
		int moo = fc14dec_init(fc_decoder,file_buffer.get_ptr(),file_buffer.get_size());
		if ( !moo ) throw exception_io_data();
		fc14dec_mixer_init(fc_decoder,44100,16,2,0x0000);
		first_block = true;
		sample_buffer.set_size( 1024  );
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		int nbSample = sample_buffer.get_size();
		fc14dec_buffer_fill(fc_decoder,sample_buffer.get_ptr(), nbSample);
		if (fc14dec_song_end(fc_decoder)) return false;
		p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), nbSample, 44100, 2,16, audio_chunk::channel_config_stereo );
		return true;
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		long seek_ms = audio_math::time_to_samples( p_seconds, 1000 );
		fc14dec_seek(fc_decoder,seek_ms);
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

	static bool g_is_our_path( const char * p_path, const char * p_extension )
	{
		return g_test_extension( p_extension );
	}

};

class mod_file_types : public input_file_type
{
	virtual unsigned get_count()
	{
		return 1;
	}

	virtual bool get_name(unsigned idx, pfc::string_base & out)
	{
		if (idx > 0) return false;
		out = "Future Composer module files";
		return true;
	}

	virtual bool get_mask(unsigned idx, pfc::string_base & out)
	{
		if (idx > 0) return false;
		out.reset();
		for (int n = 0; n < tabsize(extensions); n++)
		{
			if (n) out.add_byte(';');
			out << "*." << extensions[n];
		}
		return true;
	}

	virtual bool is_associatable(unsigned idx)
	{
		return true;
	}
};


static input_singletrack_factory_t< input_fc >   g_input_factory_fc;
static service_factory_single_t  <mod_file_types>g_input_file_type_fc_factory;

#define MYVERSION "0.2"

DECLARE_COMPONENT_VERSION("Future Composer Decoder",
	MYVERSION,
	pfc::stringcvt::string_utf8_from_os(L"A Future Composer module player for foobar2000 1.1 ->\nWritten by mudlord\n Decoding code by Jochen Hippel"));
VALIDATE_COMPONENT_FILENAME("foo_input_fc.dll");
