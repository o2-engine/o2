add_library(libpng STATIC)
file(GLOB_RECURSE libpng_SOURCES "*.c" "*.h")
list(FILTER libpng_SOURCES EXCLUDE REGEX "pngtest\\.c$")
# ../../ serves legacy "3rdPartyLibs/libpng/png.h" includes; the current dir serves
# plain <png.h> so sources build identically against bundled libpng or cocos ext_png
target_include_directories(libpng PUBLIC ../../ ${CMAKE_CURRENT_SOURCE_DIR})
target_sources(libpng PRIVATE ${libpng_SOURCES})
