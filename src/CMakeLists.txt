target_include_directories(${PROJECT_NAME} PUBLIC "include/")
# Define a library for module2
add_subdirectory("engine/")
add_subdirectory("include/")
add_subdirectory("utils/")
add_subdirectory("shaders/")
