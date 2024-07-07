cmake_minimum_required(VERSION 3.14)
project(renderer C)

# Set the C standard
set(CMAKE_C_STANDARD 17)

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


# Gather all source files
file(GLOB_RECURSE ROOT_SOURCES
    "${CMAKE_SOURCE_DIR}/main_vulkan.c"
    "${CMAKE_SOURCE_DIR}/main.c"
)

# Gather all header files
file(GLOB_RECURSE ROOT_HEADERS
    "${CMAKE_SOURCE_DIR}/main_vulkan.h"
    "${CMAKE_SOURCE_DIR}/langexceptions.h"
)

# Define the executable target
add_executable(${PROJECT_NAME})

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Include subdirectories
add_subdirectory(src)
add_subdirectory(include)

target_sources(${PROJECT_NAME}
    PRIVATE
    ${ROOT_SOURCES}
    ${ROOT_HEADERS}
)

# Add include directories to the target
target_include_directories(${PROJECT_NAME}
    PUBLIC
    ${CMAKE_SOURCE_DIR}/include
    ${SDL2_INCLUDE_DIRS}
)

# Link libraries to the executable target
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    SDL2::SDL2
    Vulkan::Vulkan
    m  # Math library
    X11  # X11 library
)

# Link rendererLib to the executable
target_link_libraries(${PROJECT_NAME}
    PRIVATE
    engine_mod
)
