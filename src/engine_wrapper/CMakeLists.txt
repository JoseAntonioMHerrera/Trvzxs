


# Gather all source files
file(GLOB_RECURSE ENGINE_WRAPPER_SOURCES
    "main.cpp",
    "model/main.cpp"
 #TODO move entryfiles to the library to engine folder tree
)

# Gather all header files
file(GLOB_RECURSE ENGINE_WRAPPER_HEADERS
     # TODO move CInterface to engine with entrypoint of engine
)


target_link_libraries(${PROJECT_NAME}
    PRIVATE
    SDL2::SDL2
    Vulkan::Vulkan
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
