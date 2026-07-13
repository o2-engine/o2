add_library(miniaudio STATIC miniaudio.c miniaudio.h)

target_include_directories(miniaudio PUBLIC "./")

if (APPLE)
    find_library(MINIAUDIO_CORE_FOUNDATION CoreFoundation)
    find_library(MINIAUDIO_CORE_AUDIO CoreAudio)
    find_library(MINIAUDIO_AUDIO_TOOLBOX AudioToolbox)
    target_link_libraries(miniaudio PUBLIC
        ${MINIAUDIO_CORE_FOUNDATION}
        ${MINIAUDIO_CORE_AUDIO}
        ${MINIAUDIO_AUDIO_TOOLBOX})
elseif (UNIX AND NOT ANDROID AND NOT EMSCRIPTEN)
    target_link_libraries(miniaudio PUBLIC dl pthread m)
endif()
