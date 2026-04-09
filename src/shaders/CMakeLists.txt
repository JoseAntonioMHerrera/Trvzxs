cmake_minimum_required(VERSION 3.12)
find_package(glslang CONFIG REQUIRED)
find_package(SPIRV-Tools CONFIG REQUIRED)
find_package(unofficial-shaderc CONFIG REQUIRED)
# Set the path to glslc compiler
# Find glslc compiler from shaderc
find_program(GLSLC glslc
    HINTS
    "${VCPKG_INSTALLED_DIR}/${VCPKG_TARGET_TRIPLET}/tools/shaderc"
    "${VCPKG_ROOT}/installed/${VCPKG_TARGET_TRIPLET}/tools/shaderc"
)

if(NOT GLSLC)
    message(FATAL_ERROR "glslc not found")
else()
    message(STATUS "Found glslc: ${GLSLC}")
endif()
# Set the shaders
set(SHADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/triangle.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/triangle.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/particle.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/particle.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/particle.comp"
    "${CMAKE_CURRENT_SOURCE_DIR}/lightVolume.vert"
    "${CMAKE_CURRENT_SOURCE_DIR}/lightVolume.frag"
    "${CMAKE_CURRENT_SOURCE_DIR}/depthPrepass.vert"

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
        COMMAND ${GLSLC} -g -o ${SPIRV} ${SHADER}
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
