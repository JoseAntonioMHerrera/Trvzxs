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

# Create library target
add_library(utils_mod STATIC ${UTILS_SOURCES} ${UTILS_HEADERS})

# Include directories
target_include_directories(utils_mod
    PUBLIC
    "fileFunctions/include"
    "include"
)
