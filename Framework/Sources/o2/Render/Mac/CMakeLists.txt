set(PET_STORY_SHADERS_SOURCES
        Shaders.metal
)

add_metal_shader_library(PetStoryShaders
        STANDARD macos-metal2.1
        ${PET_STORY_SHADERS_SOURCES}
)