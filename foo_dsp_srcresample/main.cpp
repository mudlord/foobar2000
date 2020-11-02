#include "../SDK/foobar2000.h"
#include "samplerate.h"

#define MYVERSION "0.1"

static pfc::string_formatter g_get_component_about()
{
	pfc::string_formatter about;
	about << "A resampler DSP for foobar2000 1.1 ->\n";
	about << "Written by mudlord.\n";
	about << "Uses libsamplerate from Erik de Castro Lopo.\n";
	return about;
}


DECLARE_COMPONENT_VERSION_COPY(
"Secret Rabbit Code Resampler",
MYVERSION,
g_get_component_about()
);
VALIDATE_COMPONENT_FILENAME("foo_dsp_srcresample.dll");