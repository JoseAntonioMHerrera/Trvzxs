# Gather all source files
file(GLOB_RECURSE ENGINE_SOURCES
    "device/*.c"
    "familyQueues/*.c"
)

# Gather all header files
file(GLOB_RECURSE ENGINE_HEADERS
    "buffer/include/*.h"
    "device/include/*.h"
    "familyQueues/include/*.h"
    "include/*.h"
    "uniformBuffers/*.h"
)

# Create library target
add_library(engine_mod STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# Include directories
target_include_directories(engine_mod PUBLIC
    buffer/include
    device/include
    familyQueues/include
    include
    uniformBuffers
)
