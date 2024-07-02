# CMakeLists.txt

cmake_minimum_required(VERSION 3.14)
project(src-shaders C)

# Find Vulkan package
find_package(Vulkan REQUIRED)

# Directory containing shaders
set(SHADER_DIR ${CMAKE_CURRENT_SOURCE_DIR})

# Output directory for compiled shaders (SPIR-V)
set(SHADER_OUTPUT_DIR ${CMAKE_BINARY_DIR}/bin)

# Create output directory if it doesn't exist
file(MAKE_DIRECTORY ${SHADER_OUTPUT_DIR})

# List of shaders to compile
set(SHADERS
    ${SHADER_DIR}/vertex.vert
    ${SHADER_DIR}/fragment.frag
)

# Compile shaders using glslc
foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV_OUTPUT ${SHADER_OUTPUT_DIR}/${SHADER_NAME}.spv)
    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${Vulkan_glslc_EXECUTABLE} -o ${SPIRV_OUTPUT} ${SHADER}
        DEPENDS ${SHADER}
    )
    list(APPEND SPIRV_BINARY_FILES ${SPIRV_OUTPUT})
endforeach()

# Create a custom target for shaders compilation
add_custom_target(shaders DEPENDS ${SPIRV_BINARY_FILES})
