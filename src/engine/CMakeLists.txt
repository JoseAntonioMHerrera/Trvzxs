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

if(NOT MSVC)
    target_compile_options(engine_mod PRIVATE -g)
endif()

target_include_directories(engine_mod PUBLIC
    buffer/include
    device/include
    familyQueues/include
    swapchain/include
    pipelines/include
    pipelines/descriptors/include
    pipelines/constants/include
    pipelines/bindless/include
    geometry/include
    commands/include # HOWTO_NEW_FILE: Remember to add the folder here
    "../../include/cglm"
    "../../include/"
    texture/include
    image/include
    uniformBuffers/include
    optimizations/include
    memory/
    lifecycle/include
    rendergraph/include
    lighting/include
    sync/include
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
    "lifecycle/*.c"
    "rendergraph/*.c"
    "lighting/*.c"
    "sync/*.c"
    "commands/*.c"
    "${CMAKE_SOURCE_DIR}/src/*.c" #TODO move entryfiles to the library to engine folder tree
)

# Vendored meshoptimizer — used for runtime LOD generation at scene load time.
# Public C API is declared in include/meshoptimizer/meshoptimizer.h and
# implemented by these .cpp files (compile as C++, expose extern "C" symbols).
list(APPEND ENGINE_SOURCES
    "${CMAKE_SOURCE_DIR}/include/meshoptimizer/simplifier.cpp"
    "${CMAKE_SOURCE_DIR}/include/meshoptimizer/vcacheoptimizer.cpp"
    "${CMAKE_SOURCE_DIR}/include/meshoptimizer/vfetchoptimizer.cpp"
    "${CMAKE_SOURCE_DIR}/include/meshoptimizer/indexgenerator.cpp"
)

# Gather all header files
file(GLOB_RECURSE ENGINE_HEADERS
    "../../include/cglm/cglm.h"
    "../../include/stb_image.h" # TODO sacar adicion de dependencias externas del engine a otro cmkaelist
    "pipelines/descriptors/include/*.h"
    "pipelines/bindless/include/*.h"
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
    "lifecycle/include/*.h"
    "rendergraph/include/*.h"
    "lighting/include/*.h"
    "sync/include/*.h"
    "pipelines/constants/include/*.h"
    "optimizations/include/*.h"
    "${CMAKE_SOURCE_DIR}/src/*.h"
    "${CMAKE_SOURCE_DIR}/src/engineCInterface.h" # TODO move CInterface to engine with entrypoint of engine
)

target_compile_definitions(engine_mod
  PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
  $<$<BOOL:${VMA_DUMP}>:VMA_DUMP>
)

target_sources(${PROJECT_NAME} PUBLIC ${ENGINE_HEADERS} )
# Create library target
target_sources(engine_mod PRIVATE ${ENGINE_SOURCES} ${ENGINE_HEADERS} ${CGLM_HEADERS})
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
    $<$<PLATFORM_ID:Linux>:m>  # Math library
    $<$<PLATFORM_ID:Linux>:X11>  # X11 library
    utils_mod
)
