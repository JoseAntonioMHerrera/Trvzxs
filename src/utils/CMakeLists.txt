# Gather all source files
file(GLOB_RECURSE UTILS_SOURCES
    "fileFunctions/*.c"
    "familyQueues/*.c"
)

# Gather all header files
file(GLOB_RECURSE UTILS_HEADERS
    "fileFunctions/include/*.h"
    "include/*.h"
)

target_sources(${PROJECT_NAME} PUBLIC ${UTILS_HEADERS})

# Create library target
add_library(utils_mod STATIC ${UTILS_SOURCES} ${UTILS_HEADERS})

# Public so every target linking utils_mod can resolve asset paths
target_compile_definitions(utils_mod PUBLIC ASSET_ROOT="${CMAKE_SOURCE_DIR}/")

# Include directories
target_include_directories(utils_mod
    PUBLIC
    "fileFunctions/include"
    "include"
)
