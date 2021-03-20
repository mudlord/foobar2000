#include "../SDK/foobar2000.h"
#include "../../../3rdparty-deps/SoundTouch/SoundTouch.h"
#include "dsp_guids.h"
#define MYVERSION "0.47"

static pfc::string_formatter g_get_component_about()
{
	pfc::string_formatter about;
	about << "A special effect DSP for foobar2000.\n\n";
	about << "Written by mountnside.\n";
	about << "Twitter: https://twitter.com/mountnside\n";
	about << "Github:  https://github.com/mountnside\n";
	about << "\n";
	about << "Portions by Jon Watte, Jezar Wakefield, Chris Snowhill.\n";
	about << "Using SoundTouch library version " << SOUNDTOUCH_VERSION << "\n";
	about << "SoundTouch (c) Olli Parviainen\n";
	about << "\n";
	about << "License: https://github.com/mountnside/foobar2000/blob/master/LICENSE.md";
	return about;
}


DECLARE_COMPONENT_VERSION_COPY(
"Effect DSP",
MYVERSION,
g_get_component_about()
);
VALIDATE_COMPONENT_FILENAME("foo_dsp_effect.dll");
