


# Gather all source files
file(GLOB_RECURSE ENGINE_WRAPPER_SOURCES
    "main.cpp"
    "metaEngine/model/*.cpp"
    "metaEngine/scene/*.cpp"
    "metaEngine/metaObject/*.cpp"
    "metaEngine/*.cpp"
 #TODO move entryfiles to the library to engine folder tree
)

# Gather all header files
file(GLOB_RECURSE ENGINE_WRAPPER_HEADERS
    "metaEngine/model/include/*.hpp"
    "metaEngine/metaObject/include/*.hpp"
    "metaEngine/include/*.hpp"
    "metaEngine/scene/include/*.hpp"
     # TODO move CInterface to engine with entrypoint of engine
)

option(RENDERDOC_LAYER "Add layer support for renderdoc" OFF)
message(STATUS "RENDERDOC_LAYER is set to: ${RENDERDOC_LAYER}")

if(RENDERDOC_LAYER)
    message(STATUS "Enabling RenderDoc layer support")
    target_compile_definitions(${PROJECT_NAME} PRIVATE RENDERDOC_ENABLED)
    target_compile_definitions(engine_mod PRIVATE RENDERDOC_ENABLED)
endif()

target_link_libraries(${PROJECT_NAME}
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
    engine_mod
    utils_mod
)
# Add include directories to the target
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include/cglm/
    ${SDL2_INCLUDE_DIRS}
    ${SDL}
    ${CMAKE_SOURCE_DIR}/src/
    metaEngine/model/include/
    metaEngine/scene/include/
    metaEngine/metaObject/include
    metaEngine/include
)


set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG")
# Link libraries to the executable target
target_compile_definitions(${PROJECT_NAME}
  PRIVATE
  $<$<CONFIG:Debug>:DEBUG>
  $<$<CONFIG:Release>:NDEBUG>
)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${ENGINE_WRAPPER_SOURCES}
    ${ENGINE_WRAPPER_HEADERS}
)
