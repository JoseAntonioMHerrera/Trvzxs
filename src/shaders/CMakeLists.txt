cmake_minimum_required(VERSION 3.12)

# Set the path to glslc compiler
find_program(GLSLC glslc HINTS ${Vulkan_INCLUDE_DIRS}/../bin)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found")
else()
    message(STATUS "Found glslc: ${GLSLC}")
endif()

# Set the shaders
set(SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/triangle.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/triangle.vert"
    # Add more shaders as needed
)

# Ensure the output directory for SPIR-V binaries exists
file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/build/shaders/bin)

# List to hold SPIR-V binary paths
set(SPIRV_BINARIES "")

# Create custom target for shader compilation
add_custom_target(shaders_mod ALL)

# Compile each shader
foreach(SHADER ${SHADERS})
    get_filename_component(SHADER_NAME ${SHADER} NAME)
    set(SPIRV "${CMAKE_SOURCE_DIR}/build/shaders/bin/${SHADER_NAME}.spv")

    add_custom_command(
        OUTPUT ${SPIRV}
        COMMAND ${GLSLC} -o ${SPIRV} ${SHADER}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME}"
    )

    # Append the generated SPIR-V binary to the list
    list(APPEND SPIRV_BINARIES ${SPIRV})

    # Print the list of SPIR-V binaries (for debugging purposes)
    message(STATUS "SPIR-V binaries: ${SPIRV_BINARIES}")
endforeach()

# Add dependencies to the custom target
add_custom_target(
    Shaders ALL
    DEPENDS ${SPIRV_BINARIES}
)

# Ensure shaders are built before your executable
add_dependencies(${PROJECT_NAME} shaders_mod)
