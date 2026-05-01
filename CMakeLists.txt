cmake_minimum_required(VERSION 3.28)

# 1. Global Settings & Silence (Do this FIRST to suppress SFML noise)
set(CMAKE_POLICY_VERSION_MINIMUM 3.5)
set(CMAKE_MESSAGE_LOG_LEVEL WARNING) 
set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS ON CACHE BOOL "" FORCE)

# ============================================================================
# MSVC DEBUG INFO FORMAT (SANTI 28/04/2026)
# ============================================================================
# Avoid MSVC fatal error C1041 (PDB contention) during parallel builds
# (commonly triggered by C deps like mbedTLS under Ninja) by using Embedded
# debug info (Z7) instead of Program Database (Zi) in Debug/RelWithDebInfo.
if (MSVC)
    set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:Embedded>")
endif()


project(SPCE LANGUAGES CXX)

# 2. Compiler Settings
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# This puts your .exe in build/bin/ instead of burying it in subfolders
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 3. Fetch SFML 3.1.0
include(FetchContent)
FetchContent_Declare(SFML
    GIT_REPOSITORY https://github.com/SFML/SFML.git
    GIT_TAG 3.1.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM
)
FetchContent_MakeAvailable(SFML)

# 4. Handle Nested Folder Structure
# GLOB_RECURSE finds everything in src/ so you don't have to list them manually.
file(GLOB_RECURSE GAME_SOURCES 
    "src/*.cpp" 
    "src/*.h" 
    "src/*.hpp"
)

# 5. Define the Target
# CLEANUP: Removed the long list of manual files. 
# Having them in both GLOB and here causes "duplicate symbol" errors.
add_executable(${PROJECT_NAME} ${GAME_SOURCES})

# 6. Include & Link
target_include_directories(${PROJECT_NAME} PRIVATE src)
target_link_libraries(${PROJECT_NAME} PRIVATE 
    SFML::Graphics 
    SFML::Audio 
    SFML::Network 
    SFML::Window 
    SFML::System
)

# 7. Asset Management
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/assets")
    # Rule A: Copy to the bin folder (Where the .exe lives)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>/assets"
    )

    # Rule B: Copy to the build root (Where Visual Studio looks by default)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "${CMAKE_BINARY_DIR}/assets"
    )
endif()

# Set VS Startup Project
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
