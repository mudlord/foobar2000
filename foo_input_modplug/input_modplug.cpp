#define _WIN32_WINNT 0x0501
#include <foobar2000.h>

#include "../helpers/dropdown_helper.h"
#include "../ATLHelpers/ATLHelpers.h"
#include "modplug.h"

// {6DDFCDA7-C5D6-42AC-9D7A-43150BF4AA76}
static const GUID guid_cfg_parent_modplug = 
{ 0x6ddfcda7, 0xc5d6, 0x42ac, { 0x9d, 0x7a, 0x43, 0x15, 0xb, 0xf4, 0xaa, 0x76 } };

// {380493D4-BC90-4598-BF7F-68F7DFDD8133}
static const GUID guid_cfg_parent_interpolation = 
{ 0x380493d4, 0xbc90, 0x4598, { 0xbf, 0x7f, 0x68, 0xf7, 0xdf, 0xdd, 0x81, 0x33 } };

// {99FCDFF1-8C64-452A-B2E2-E3A98E936C9A}
static const GUID guid_cfg_loop = 
{ 0x99fcdff1, 0x8c64, 0x452a, { 0xb2, 0xe2, 0xe3, 0xa9, 0x8e, 0x93, 0x6c, 0x9a } };

// {E6BF7CD7-5E4E-4A8A-993D-46D444A102CA}
static const GUID guid_cfg_interpolation_none = 
{ 0xe6bf7cd7, 0x5e4e, 0x4a8a, { 0x99, 0x3d, 0x46, 0xd4, 0x44, 0xa1, 0x2, 0xca } };
// {943ECA40-D2C5-4122-B608-1C83019844C6}
static const GUID guid_cfg_interpolation_linear = 
{ 0x943eca40, 0xd2c5, 0x4122, { 0xb6, 0x8, 0x1c, 0x83, 0x1, 0x98, 0x44, 0xc6 } };
// {0B5A8D5A-9947-4D4C-923F-5CFE36539691}
static const GUID guid_cfg_interpolation_cubic = 
{ 0xb5a8d5a, 0x9947, 0x4d4c, { 0x92, 0x3f, 0x5c, 0xfe, 0x36, 0x53, 0x96, 0x91 } };
// {63239516-4DCD-4539-BB72-CB41AED9437A}
static const GUID guid_cfg_interpolation_sinc = 
{ 0x63239516, 0x4dcd, 0x4539, { 0xbb, 0x72, 0xcb, 0x41, 0xae, 0xd9, 0x43, 0x7a } };


advconfig_branch_factory cfg_organya_parent("ModPlug decoder", guid_cfg_parent_modplug, advconfig_branch::guid_branch_playback, 0);
advconfig_branch_factory cfg_interpolation_parent("Interpolation method", guid_cfg_parent_interpolation, guid_cfg_parent_modplug, 1.0);
advconfig_checkbox_factory cfg_loop("Loop indefinitely", guid_cfg_loop, guid_cfg_parent_modplug, 0, false);
advconfig_radio_factory cfg_interpolation_none("None", guid_cfg_interpolation_none, guid_cfg_parent_interpolation, 0, true);
advconfig_radio_factory cfg_interpolation_linear("Linear", guid_cfg_interpolation_linear, guid_cfg_parent_interpolation, 1, false);
advconfig_radio_factory cfg_interpolation_cubic("Cubic", guid_cfg_interpolation_cubic, guid_cfg_parent_interpolation, 2, false);
advconfig_radio_factory cfg_interpolation_sinc("Sinc", guid_cfg_interpolation_sinc, guid_cfg_parent_interpolation, 3, false);

static const char * extensions[]=
{
	"669",
	"MOD",
	"S3M",
	"PTM",
	"IT",
	"XM",
	"MTM",
	"MED",
	"669",
	"ULT",
	"STM",
	"FAR",
	"AMF",
	"ABC",
	"OKT",
	"DSM",
	"MT2",
	"PSM"
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

class input_modplug
{
	ModPlugFile* m_player;
	t_filestats m_stats;
	long length;
	int is_playing;
	bool first_block,loop;
	pfc::array_t< t_int16 > sample_buffer;
	pfc::array_t< t_uint8 > file_buffer;
public:
	input_modplug()
	{
	}

	~input_modplug()
	{
		if(m_player)ModPlug_Unload(m_player);
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
		m_player = ModPlug_Load(ptr, size);
		if ( !m_player ) throw exception_io_data();
	}

	void get_info( file_info & p_info, abort_callback & p_abort )
	{
		ModPlugFile* m_info = ModPlug_Load(file_buffer.get_ptr(), file_buffer.get_size());
		p_info.info_set( "encoding", "synthesized" );
		int type_module = ModPlug_GetModuleType(m_info);
		p_info.info_set( "codec", "Module file" );
		p_info.info_set_int( "channels", 2 );
		p_info.meta_set( "title", pfc::stringcvt::string_utf8_from_ansi(  ModPlug_GetName(m_info)  ));
		int len = ModPlug_GetLength(m_info);
		len /= 1000;
		p_info.set_length( len );
		if(m_info)ModPlug_Unload(m_info);
	}

	t_filestats get_file_stats( abort_callback & p_abort )
	{
		return m_stats;
	}

	void decode_initialize( unsigned p_flags, abort_callback & p_abort )
	{
		ModPlug_Settings settings;
		ModPlug_GetSettings(&settings);
		settings.mFrequency = 44100;
		settings.mBits = 32;
		unsigned interpolation_method = cfg_interpolation_none ? 0 : cfg_interpolation_cubic ? 2 : cfg_interpolation_sinc ? 3 : cfg_interpolation_linear ? 1 : 0;
		settings.mResamplingMode = interpolation_method;
		settings.mChannels = 2;
		bool dont_loop = !cfg_loop || !! ( p_flags & input_flag_no_looping );
		if ( dont_loop) 
		{
			settings.mLoopCount = 0;
			loop = false;
		}
		else
		{
			settings.mLoopCount = -1; // endless
			loop = true;
		}
		ModPlug_SetSettings(&settings);
		first_block = true;
		is_playing = 1;
	}

	bool decode_run( audio_chunk & p_chunk, abort_callback & p_abort )
	{
		if (!loop && is_playing == 0) return false;
		int nbSample = 512;
		sample_buffer.grow_size( nbSample );
		is_playing = ModPlug_Read(m_player, sample_buffer.get_ptr(), nbSample*2);
		p_chunk.set_data_fixedpoint( sample_buffer.get_ptr(), nbSample * 2, 44100, 2, 32, audio_chunk::channel_config_stereo );
		return true;
	}

	void decode_seek( double p_seconds,abort_callback & p_abort )
	{
		long seek_ms = audio_math::time_to_samples( p_seconds, 1000 );
		ModPlug_Seek(m_player,seek_ms);
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
		return ! strcmp( p_content_type, "audio/x-mod" );
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
		out = "Module files";
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


static input_singletrack_factory_t< input_modplug >         g_input_factory_modplug;
static service_factory_single_t  <mod_file_types>           g_input_file_type_modplug_factory;

#define MYVERSION "0.2"

DECLARE_COMPONENT_VERSION("ModPlug Decoder",
	MYVERSION,
	pfc::stringcvt::string_utf8_from_os(L"A module player for foobar2000 1.1 ->\nWritten by mudlord\nModPlug library by Olivier Lapicque"));
VALIDATE_COMPONENT_FILENAME("foo_input_modplug.dll");
