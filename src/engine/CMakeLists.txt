add_library(engine_mod)
add_subdirectory("include/")
target_include_directories(engine_mod PUBLIC
    buffer/include
    device/include
    familyQueues/include
    swapchain/include
    pipelines/include
    geometry/include
    commands/include # HOWTO_NEW_FILE: Remember to add the folder here
    "../../include/cglm"
    uniformBuffers/include
    "include/"
)

# Gather all source files
file(GLOB_RECURSE ENGINE_SOURCES
    "device/*.c"
    "familyQueues/*.c"
)

# Gather all header files
file(GLOB_RECURSE ENGINE_HEADERS
    "device/include/*.h"
    "buffer/include/*.h"
    "familyQueues/include/*.h"
    "swapchain/include/*.h"
    "geometry/include/*.h"
    "pipelines/include/*.h"
    "commands/include/*.h"
)

target_sources(${PROJECT_NAME} PUBLIC ${ENGINE_HEADERS})
# Create library target
target_sources(engine_mod PUBLIC ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${CGLM_HEADERS})
# Link libraries to the executable target
target_link_libraries(engine_mod
    PRIVATE
    SDL2::SDL2
    Vulkan::Vulkan
    m  # Math library
    X11  # X11 library
    utils_mod
)
