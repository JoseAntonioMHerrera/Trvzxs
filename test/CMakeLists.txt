find_package(Vulkan REQUIRED)

add_executable(test_scene_lifecycle
    test_scene_lifecycle.c
    test_helpers.c
)
target_link_libraries(test_scene_lifecycle PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_scene_lifecycle PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_ubo_compaction
    test_ubo_compaction.c
    test_helpers.c
)
target_link_libraries(test_ubo_compaction PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_ubo_compaction PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_light_lifecycle
    test_light_lifecycle.c
    test_helpers.c
)
target_link_libraries(test_light_lifecycle PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_light_lifecycle PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_render_graph
    test_render_graph.c
)
target_link_libraries(test_render_graph PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_render_graph PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

# Run all test suites sequentially
add_custom_target(run_all_tests
    COMMAND test_scene_lifecycle
    COMMAND test_ubo_compaction
    COMMAND test_light_lifecycle
    COMMAND test_render_graph
    DEPENDS test_scene_lifecycle test_ubo_compaction test_light_lifecycle test_render_graph
    COMMENT "Running all test suites..."
)
