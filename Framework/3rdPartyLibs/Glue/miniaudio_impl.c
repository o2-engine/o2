// miniaudio with the Vorbis decoder of its extras (stb_vorbis) in one translation unit; its options are compile
// definitions of the miniaudio target
#define STB_VORBIS_HEADER_ONLY
#include "../miniaudio/extras/stb_vorbis.c"

#include "../miniaudio/miniaudio.c"

#undef STB_VORBIS_HEADER_ONLY
#include "../miniaudio/extras/stb_vorbis.c"
