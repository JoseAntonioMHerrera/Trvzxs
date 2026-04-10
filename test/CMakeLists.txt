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

add_executable(test_dense_array
    test_dense_array.c
    test_helpers.c
)
target_link_libraries(test_dense_array PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_dense_array PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_frame_safety
    test_frame_safety.c
    test_helpers.c
)
target_link_libraries(test_frame_safety PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_frame_safety PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_light_frame_safety
    test_light_frame_safety.c
    test_helpers.c
)
target_link_libraries(test_light_frame_safety PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_light_frame_safety PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_light_ubo_compaction
    test_light_ubo_compaction.c
    test_helpers.c
)
target_link_libraries(test_light_ubo_compaction PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_light_ubo_compaction PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_instance_hierarchy
    test_instance_hierarchy.c
    test_helpers.c
)
target_link_libraries(test_instance_hierarchy PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_instance_hierarchy PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

add_executable(test_frustum
    test_frustum.c
    test_helpers.c
)
target_link_libraries(test_frustum PRIVATE engine_mod Vulkan::Headers)
target_include_directories(test_frustum PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    $<TARGET_PROPERTY:engine_mod,INCLUDE_DIRECTORIES>
)

# Run all test suites sequentially
add_custom_target(run_all_tests
    COMMAND test_scene_lifecycle
    COMMAND test_ubo_compaction
    COMMAND test_light_lifecycle
    COMMAND test_dense_array
    COMMAND test_frame_safety
    COMMAND test_light_frame_safety
    COMMAND test_light_ubo_compaction
    COMMAND test_instance_hierarchy
    COMMAND test_frustum
    COMMAND test_render_graph
    DEPENDS test_scene_lifecycle test_ubo_compaction test_light_lifecycle test_dense_array test_frame_safety test_light_frame_safety test_light_ubo_compaction test_instance_hierarchy test_frustum test_render_graph
    COMMENT "Running all test suites..."
)
