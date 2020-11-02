#include "../SDK/foobar2000.h"
#include "libv2/types.h"
#include "libv2/v2mplayer.h"

class input_v2m {
  private:
  V2MPlayer player;
  service_ptr_t<file> m_file;
  pfc::array_t<t_uint8> m_buffer;
  double m_length;

  public:

    input_v2m()
    {
		 player.Init();
    }

    ~input_v2m()
    {
		player.Close();
		
		

    }

    void open(service_ptr_t<file> p_filehint, const char *p_path, t_input_open_reason p_reason, abort_callback &p_abort)
    {
	  pfc::array_t<t_uint8> m_temp;
      if (p_reason == input_open_info_write)
        throw exception_io_unsupported_format();
      m_file = p_filehint;
      input_open_file_helper(m_file, p_path, p_reason, p_abort);
      t_filesize size = m_file->get_size(p_abort);
      if (size == filesize_invalid)
        throw exception_io_no_length();
      if (size > 128 * 1024 * 1024)
        throw pfc::exception_overflow();
      m_temp.set_size((t_size)size);
      m_file->read(m_temp.get_ptr(), (t_size)size, p_abort);
	  double len = 0;
	 
	  if (!player.Open(m_temp.get_ptr(),44100))
      throw exception_io_unsupported_format();
	  m_length = len;
    }

    void get_info(file_info &p_info, abort_callback &p_abort)
    {
      p_info.set_length(m_length);
      p_info.info_set_int("samplerate", 44100);
      p_info.info_set_int("channels", 2);
      p_info.info_set_int("bitspersample", 16);
      p_info.info_set("encoding", "synthesized");
    }

    t_filestats get_file_stats(abort_callback &p_abort)
    {
      return m_file->get_stats(p_abort);
    }

    void decode_initialize(unsigned p_flags, abort_callback &p_abort)
    {
		player.Play(0);
    }

    bool decode_run(audio_chunk &p_chunk, abort_callback &p_abort)
    {
      p_chunk.set_srate(44100);
      p_chunk.set_channels(2);
      p_chunk.set_data_size(1024 * 2);
      pfc::assert_same_type<audio_sample, float>;
	  player.Render(p_chunk.get_data(),1024);
	  if (!player.IsPlaying())
	  {
		  p_chunk.set_sample_count(0);
		  //EOF
		  return false;
	  }
	  else
	  {
		  p_chunk.set_sample_count(1024);
		  //processed successfully, no EOF
		  return true;
	  }
    }

    void decode_seek(double p_seconds, abort_callback &p_abort)
    {
	 player.Play(p_seconds);
    }

    bool decode_can_seek()
    {
      return true;
    }

    bool decode_get_dynamic_info(file_info &p_out, double &p_timestamp_delta)
    {
      return false;
    }

    bool decode_get_dynamic_info_track(file_info &p_out, double &p_timestamp_delta)
    {
      return false;
    }

    void decode_on_idle(abort_callback & p_abort)
    {
    }

    void retag(const file_info &p_info, abort_callback &p_abort)
    {
      throw exception_io_unsupported_format();
    }

    static bool g_is_our_content_type(const char *p_content_type)
    {
      return false;
    }

    static bool g_is_our_path(const char *p_path, const char *p_extension)
    {
      return stricmp_utf8(p_extension, "v2m") == 0;
    }
};

static input_singletrack_factory_t<input_v2m> g_input_v2m_factory;

#define MYVERSION "0.2"
DECLARE_COMPONENT_VERSION("V2M Decoder",
MYVERSION,
pfc::stringcvt::string_utf8_from_os(L"A module player for foobar2000 1.1 ->\nWritten by mudlord\nV2 synthesizer by Tammo 'kb' Hinrichs"));
VALIDATE_COMPONENT_FILENAME("foo_input_v2m.dll");
DECLARE_FILE_TYPE("V2 module", "*.v2m");
