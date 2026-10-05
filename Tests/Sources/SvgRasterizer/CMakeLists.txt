# Command line SVG rasterizer over the framework vector graphics core: no Application, no window
add_executable(o2SvgRasterizer SvgRasterizerMain.cpp)

target_include_directories(o2SvgRasterizer PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Framework/Sources
)

target_link_libraries(o2SvgRasterizer PRIVATE
    o2Framework
)

add_dependencies(o2SvgRasterizer o2Framework o2FrameworkCodegen)

if(MSVC)
    target_compile_options(o2SvgRasterizer PRIVATE ${O2_MSVC_MP_FLAG} "/Zc:__cplusplus" "/bigobj")
endif()

set_target_properties(o2SvgRasterizer PROPERTIES FOLDER o2/Tests)
