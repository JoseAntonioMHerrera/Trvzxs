add_library(engine_mod)
add_subdirectory("include/")

find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(VulkanHeaders CONFIG REQUIRED)
find_package(VulkanMemoryAllocator CONFIG REQUIRED)
find_package(glslang CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)

target_compile_options(engine_mod PRIVATE -g)

target_include_directories(engine_mod PUBLIC
    buffer/include
    device/include
    familyQueues/include
    swapchain/include
    pipelines/include
    pipelines/descriptors/include
    pipelines/constants/include
    geometry/include
    commands/include # HOWTO_NEW_FILE: Remember to add the folder here
    "../../include/cglm"
    "../../include/"
    texture/include
    image/include
    uniformBuffers/include
    optimizations/include
    memory/
    "include/"
    "display/include"
    ${CMAKE_SOURCE_DIR}/src/ #TODO move entryfiles to the library to engine folder tree
)

# Gather all source files
file(GLOB_RECURSE ENGINE_SOURCES
    "buffer/*.c"
    "commands/*.c"
    "device/*.c"
    "display/*.c"
    "geometry/*.c"
    "image/*.c"
    "pipelines/*.c"
    "pipelines/descriptors/*.c"
    "pipelines/constants/*.c"
    "swapchain/*.c"
    "texture/*.c"
    "memory/*.cpp"
    "commands/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.c" #TODO move entryfiles to the library to engine folder tree
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
    "display/include/*.h"
    "pipelines/constants/include/*.h"
    "optimizations/include/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/engineCInterface.h" # TODO move CInterface to engine with entrypoint of engine
)

target_compile_definitions(engine_mod
  PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

target_sources(${PROJECT_NAME} PUBLIC ${ENGINE_HEADERS} )
# Create library target
target_sources(engine_mod PUBLIC ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${CGLM_HEADERS})
# Link libraries to the executable target
target_link_libraries(engine_mod
    PRIVATE
    $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
    $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
    $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>

    # Vulkan and related
    Vulkan::Vulkan
    Vulkan::Headers
    GPUOpen::VulkanMemoryAllocator

    # SPIR-V and shader compilation
    glslang::glslang
    glslang::SPIRV
    SPIRV-Tools-static
    unofficial::shaderc::shaderc
    m  # Math library
    X11  # X11 library
    utils_mod
)
