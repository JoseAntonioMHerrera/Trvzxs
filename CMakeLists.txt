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

# FetchContent for SDL2_ttf
FetchContent_Declare(
    SDL2_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf
    GIT_TAG release-2.20.2
    GIT_PROGRESS TRUE
)
set(SDL2TTF_SAMPLES OFF CACHE BOOL "" FORCE)
set(SDL2TTF_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(SDL2_ttf)

# Set the C standard
set(CMAKE_C_STANDARD 17)

# Find Vulkan package
find_package(Vulkan REQUIRED)

set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(${PROJECT_NAME})
# iterate CMakeLists
add_subdirectory(src)
add_subdirectory(include)
