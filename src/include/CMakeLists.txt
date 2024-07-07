include_directories(geometry)
include_directories(langDefinitions)
# Gather all header files
file(GLOB_RECURSE INCLUDE_HEADERS
    "geometry/*.h"
    "langDefinitions/*.h"
    "router.h" #HOWTO_NEW_FILE: Add new .h in src/include here, go and add some
)
target_include_directories(${PROJECT_NAME} PUBLIC "geometry/" "langDefinitions/")
target_sources(${PROJECT_NAME} PUBLIC ${INCLUDE_HEADERS})
