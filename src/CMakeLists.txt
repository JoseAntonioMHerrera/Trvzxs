target_include_directories(${PROJECT_NAME} PUBLIC
"utils/"
"engine/"
"shaders/"
)
# Define a library for module2
add_subdirectory("utils/")
add_subdirectory("engine/")
add_subdirectory("shaders/")
