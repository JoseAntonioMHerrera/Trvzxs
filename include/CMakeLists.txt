include_directories("cglm/")
file(GLOB_RECURSE CGLM_HEADERS "${CMAKE_SOURCE_DIR}/include/cglm/*.h")
file(GLOB_RECURSE STB_HEADERS "${CMAKE_SOURCE_DIR}/include/cglm/*.h")
target_include_directories(${PROJECT_NAME}
    PUBLIC
    "cglm/"
    .
)
