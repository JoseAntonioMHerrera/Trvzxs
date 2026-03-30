add_executable(test_scene_lifecycle
    test_scene_lifecycle.c
    test_helpers.c
)
find_package(Vulkan REQUIRED)
target_link_libraries(test_scene_lifecycle PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_scene_lifecycle PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
