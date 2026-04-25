cmake_minimum_required(VERSION 3.28)

# FIX: Crucial for CMake 4.x users to support the FreeType dependency
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)

project(SPCE LANGUAGES CXX)

# 1. Compiler Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 2. Fetch SFML 3.1.0
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

# 3. Handle Nested Folder Structure
# This looks into all subdirectories (Core, Network, etc.) for code files
file(GLOB_RECURSE GAME_SOURCES 
    "src/*.cpp" 
    "src/*.h" 
    "src/*.hpp"
)

# 4. Define the Target
add_executable(${PROJECT_NAME} ${GAME_SOURCES})

# 5. The "EE-Approved" Include Strategy
# This allows you to #include "Common/Types.h" instead of "../Common/Types.h"
target_include_directories(${PROJECT_NAME} PRIVATE src)

# 6. Link SFML Modules
# Including Network and Audio as required by your project structure
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SFML::Graphics 
    SFML::Audio 
    SFML::Network 
    SFML::Window 
    SFML::System
)

# 7. Quality of Life: Asset Management
# Automatically copies your 'assets' folder to the bin folder
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    )
endif()

# 8. Windows Deployment: Copy DLLs
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()

# Set VS Startup Project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})