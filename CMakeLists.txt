cmake_minimum_required(VERSION 3.15)
project(PathfindingVisualization VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Platform detection
if(WIN32)
    add_compile_definitions(PLATFORM_WINDOWS)
    message(STATUS "Platform: Windows")
elseif(UNIX AND NOT APPLE)
    add_compile_definitions(PLATFORM_LINUX)
    message(STATUS "Platform: Linux")
elseif(APPLE)
    add_compile_definitions(PLATFORM_MACOS)
    message(STATUS "Platform: macOS")
endif()

# Find dependencies
find_package(SDL2 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(OpenCL REQUIRED)

# ImGui (Docking Branch)
include(FetchContent)
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui
    GIT_TAG docking
)
FetchContent_MakeAvailable(imgui)

# Create ImGui library target
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC 
    ${imgui_SOURCE_DIR} 
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
)
target_link_libraries(imgui PUBLIC 
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
)

# GLM is header-only, so we just need to find it
find_package(glm QUIET)
if(NOT glm_FOUND)
    # If glm is not found via CMake, assume it's in system includes
    message(STATUS "GLM not found via CMake, assuming system installation")
endif()

# Collect all source files
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/**/*.cpp"
)

# Create executable
add_executable(pathfinding ${SOURCES})

# Include directories
target_include_directories(pathfinding PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${SDL2_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${GLEW_INCLUDE_DIRS}
    ${OpenCL_INCLUDE_DIRS}
)

# Link libraries
target_link_libraries(pathfinding PRIVATE
    ${SDL2_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GLEW_LIBRARIES}
    ${OpenCL_LIBRARIES}
    imgui
)

# Platform-specific linking
if(UNIX AND NOT APPLE)
    # Linux: Link EGL and DL
    target_link_libraries(pathfinding PRIVATE EGL dl)
endif()

if(WIN32)
    # Windows: Additional libraries if needed
    # WGL is part of OpenGL32
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(pathfinding PRIVATE /W4)
else()
    target_compile_options(pathfinding PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Optimization flags for Release
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    if(MSVC)
        target_compile_options(pathfinding PRIVATE /O2)
    else()
        target_compile_options(pathfinding PRIVATE -O3)
    endif()
endif()

# Copy assets to build directory
add_custom_command(TARGET pathfinding POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets
        $<TARGET_FILE_DIR:pathfinding>/assets
    COMMENT "Copying assets to build directory"
)

# Copy Python script to build directory
add_custom_command(TARGET pathfinding POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_SOURCE_DIR}/scripts/gen_maze.py
        $<TARGET_FILE_DIR:pathfinding>/gen_maze.py
    COMMENT "Copying gen_maze.py to build directory"
)

# Install targets (optional)
install(TARGETS pathfinding DESTINATION bin)
install(DIRECTORY assets DESTINATION bin)
install(FILES scripts/gen_maze.py DESTINATION bin)
