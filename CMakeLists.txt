cmake_minimum_required(VERSION 3.24)
project(AudioVisualizer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_OSX_ARCHITECTURES "arm64")

include(FetchContent)

message(STATUS "Fetching Raylib...")
FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
)
FetchContent_MakeAvailable(raylib)

message(STATUS "Fetching miniaudio...")
FetchContent_Declare(
    miniaudio
    GIT_REPOSITORY https://github.com/mackron/miniaudio.git
    GIT_TAG 0.11.21
)

FetchContent_MakeAvailable(miniaudio)

file(GLOB_RECURSE SOURCES 
    "src/*.cpp" 
    "src/*.h"
)

add_executable(${PROJECT_NAME} ${SOURCES}
        src/ParticleGenerator.cpp)

target_include_directories(${PROJECT_NAME} PRIVATE src)
target_include_directories(${PROJECT_NAME} PRIVATE ${miniaudio_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE raylib)

# macOS Specific: Link Core Audio frameworks
if (APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE 
        "-framework CoreAudio"
        "-framework AudioToolbox" 
        "-framework IOKit"
        "-framework Cocoa"
    )
endif()

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/demos"
    "$<TARGET_FILE_DIR:${PROJECT_NAME}>/demos"
    COMMENT "Copying audio assets to build directory..."
)