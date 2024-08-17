cmake_minimum_required(VERSION 3.14)
project(renderer LANGUAGES C CXX)

# FetchContent for SDL2
include(FetchContent)
FetchContent_Declare(
    SDL2
    GIT_REPOSITORY  https://github.com/libsdl-org/SDL
    GIT_TAG 92fe3b19c868ad062c323dde2cfc9d8b4bfdd785
    GIT_PROGRESS    TRUE
)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_SHARED ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2)

# Set the C standard
set(CMAKE_C_STANDARD 17)

# Gather all source files
file(GLOB_RECURSE ROOT_SOURCES
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
)

# Find Vulkan package
find_package(Vulkan REQUIRED)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 17)

add_executable(${PROJECT_NAME})



# iterate CMakeLists
add_subdirectory(src)
add_subdirectory(include)




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
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    SDL2::SDL2
    Vulkan::Vulkan
    m  # Math library
    X11  # X11 library
    engine_mod
    utils_mod
)


target_sources(${PROJECT_NAME}
    PRIVATE
    ${ROOT_SOURCES}
    ${ROOT_HEADERS}
)
