add_library(miniaudio STATIC miniaudio.c miniaudio.h)

target_include_directories(miniaudio PUBLIC "./")

# Vendored third-party code: never fail the build on toolchain warnings
if (NOT MSVC)
    target_compile_options(miniaudio PRIVATE -Wno-error)
endif()

if (APPLE)
    find_library(MINIAUDIO_CORE_FOUNDATION CoreFoundation)
    find_library(MINIAUDIO_CORE_AUDIO CoreAudio)
    find_library(MINIAUDIO_AUDIO_TOOLBOX AudioToolbox)
    target_link_libraries(miniaudio PUBLIC
        ${MINIAUDIO_CORE_FOUNDATION}
        ${MINIAUDIO_CORE_AUDIO}
        ${MINIAUDIO_AUDIO_TOOLBOX})

    # On mobile Apple miniaudio manages AVAudioSession, so it must be compiled as Objective-C
    if (CMAKE_SYSTEM_NAME STREQUAL "iOS")
        enable_language(OBJC)
        set_source_files_properties(miniaudio.c PROPERTIES LANGUAGE OBJC)

        find_library(MINIAUDIO_AVFOUNDATION AVFoundation)
        target_link_libraries(miniaudio PUBLIC ${MINIAUDIO_AVFOUNDATION})
    endif()
elseif (UNIX AND NOT ANDROID AND NOT EMSCRIPTEN)
    target_link_libraries(miniaudio PUBLIC dl pthread m)
endif()
