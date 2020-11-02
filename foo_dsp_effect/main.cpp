#include "../SDK/foobar2000.h"
#include "SoundTouch/SoundTouch.h"
#include "dsp_guids.h"
#define MYVERSION "0.43"

static pfc::string_formatter g_get_component_about()
{
	pfc::string_formatter about;
	about << "A special effect DSP for foobar2000.\n";
	about << "Written by mudlord.\n";
	about << "Portions by Jon Watte, Jezar Wakefield, Chris Snowhill.\n";
	about << "Using SoundTouch library version " << SOUNDTOUCH_VERSION << "\n";
	about << "SoundTouch (c) Olli Parviainen\n";
	about << "\n";
	about << "License: https://github.com/mudlord/foobar2000-plugins/blob/master/LICENSE.md";
	return about;
}


DECLARE_COMPONENT_VERSION_COPY(
"Effect DSP",
MYVERSION,
g_get_component_about()
);
VALIDATE_COMPONENT_FILENAME("foo_dsp_effect.dll");
