add_library(pl_mpeg STATIC pl_mpeg.c pl_mpeg.h)

target_include_directories(pl_mpeg PUBLIC "./")

# Vendored third-party code: never fail the build on toolchain warnings
if (NOT MSVC)
    target_compile_options(pl_mpeg PRIVATE -Wno-error)
endif()
