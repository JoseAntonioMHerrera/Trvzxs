add_library(engine_mod)
add_subdirectory("include/")
target_include_directories(engine_mod PUBLIC
    buffer/include
    device/include
    familyQueues/include
    swapchain/include
    pipelines/include
    pipelines/descriptors/include
    geometry/include
    commands/include # HOWTO_NEW_FILE: Remember to add the folder here
    "../../include/cglm"
    "../../include/"
    texture/include
    image/include
    uniformBuffers/include
    "include/"
)

# Gather all source files
file(GLOB_RECURSE ENGINE_SOURCES
    "device/*.c"
    "familyQueues/*.c"
    "commands/*.c"
    "image/*.c"
    "display/*.c"
    "texture/*.c"
    "pipelines/descriptors/*.c"
)

# Gather all header files
file(GLOB_RECURSE ENGINE_HEADERS
    "../../include/cglm/cglm.h"
    "../../include/stb_image.h" # TODO sacar adicion de dependencias externas del engine a otro cmkaelist
    "pipelines/descriptors/include/*.h"
    "device/include/*.h"
    "buffer/include/*.h"
    "familyQueues/include/*.h"
    "swapchain/include/*.h"
    "geometry/include/*.h"
    "pipelines/include/*.h"
    "commands/include/*.h"
    "geometry/include/*.h"
    "texture/include/*.h"
    "image/include/*.h"
)

target_sources(${PROJECT_NAME} PUBLIC ${ENGINE_HEADERS} )
# Create library target
target_sources(engine_mod PUBLIC ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${CGLM_HEADERS} ${STB_HEADERS})
# Link libraries to the executable target
target_link_libraries(engine_mod
    PRIVATE
    SDL2::SDL2
    Vulkan::Vulkan
    m  # Math library
    X11  # X11 library
    utils_mod
)
