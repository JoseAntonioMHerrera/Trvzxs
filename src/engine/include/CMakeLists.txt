include_directories(geometry)
include_directories(langDefinitions)
# Gather all header files
file(GLOB_RECURSE INCLUDE_HEADERS
    "langDefinitions/*.h"
    "*.h" #HOWTO_NEW_FILE: Add new .h in src/include here, go and add some

)
target_include_directories(engine_mod PUBLIC "langDefinitions/")
target_include_directories(engine_mod PUBLIC "langDefinitions/")
target_sources(engine_mod PUBLIC ${INCLUDE_HEADERS})
