cmake_minimum_required(VERSION 3.24)

# =============================================================================
# Project Configuration
# =============================================================================
project(VirtualPhonePro
    VERSION 1.0.0
    DESCRIPTION "Commercial-grade anti-detect Android emulator manager"
    HOMEPAGE_URL "https://github.com/mostakimnasim5/VirtualPhonePro"
    LANGUAGES CXX
)

# C++ Standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =============================================================================
# Qt6 Configuration
# =============================================================================

# Try to find Qt6 using CMake's Qt6 components
set(QT_MIN_VERSION 6.5.0)

# First try: Qt6 in system (vcpkg/conan installed)
find_package(Qt6 COMPONENTS
    Core
    Network
    Process
    Widgets
    Gui
    Xml
    Sql
    QUIET
)

if(NOT Qt6_FOUND)
    # Second try: Qt6 via qt6-core CMake package
    find_package(Qt6Core QUIET)
    if(Qt6Core_FOUND)
        set(Qt6_FOUND TRUE)
        # Use Qt6Core's CMake helpers
        include(${Qt6Core_CMAKE_DIR}/Qt6AndroidMacros.cmake)
    endif()
endif()

if(NOT Qt6_FOUND)
    message(FATAL_ERROR "
        Qt6 not found. Please install Qt6 using one of:
        
        1. vcpkg: vcpkg install qt6-base
        2. Conan: conan install qt/6.x.x
        3. Download Qt6 from https://www.qt.io/download-qt-installer
        
        Ensure Qt6_DIR or Qt6Core_DIR is set to Qt6 installation directory.
    ")
endif()

message(STATUS "Found Qt6: ${Qt6_DIR}")
message(STATUS "Qt6 Version: ${Qt6_VERSION}")

# =============================================================================
# Platform Detection
# =============================================================================

if(WIN32)
    set(PLATFORM_WINDOWS TRUE)
    message(STATUS "Platform: Windows")
elseif(UNIX AND NOT APPLE)
    set(PLATFORM_LINUX TRUE)
    message(STATUS "Platform: Linux")
elseif(APPLE)
    set(PLATFORM_MACOS TRUE)
    message(STATUS "Platform: macOS")
endif()

# =============================================================================
# Build Type Configuration
# =============================================================================

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "RelWithDebInfo" "MinSizeRel")
endif()

# Compiler-specific optimizations
if(MSVC)
    add_compile_options(/W4 /WX-)  # Warning level 4, warnings as errors disabled
    add_compile_options(/utf-8)     # UTF-8 source files
    add_compile_options(/permissive-) # Strict conformance mode
    
    # Performance optimizations
    add_compile_options(/O2 /Ob2 /Oi /Ot /GF /Gy)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS WIN32_LEAN_AND_MEAN)
    
    # Disable specific warnings that may fire on valid code
    add_compile_options(/wd4251 /wd4244 /wd4267)
    
    # Linker optimizations
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /OPT:REF /OPT:ICF")
    
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic)
    add_compile_options(-O3 -march=native -mtune=native)
    
    # Stack protector for security
    add_compile_options(-fstack-protector-strong)
    
    # Position independent code
    add_compile_options(-fPIC)
endif()

# =============================================================================
# Output Directories
# =============================================================================

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb)

# Install directories
set(CMAKE_INSTALL_PREFIX "C:/Program Files/VirtualPhonePro" CACHE PATH "Installation prefix")

# =============================================================================
# Source Files
# =============================================================================

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
)

# Core source files
set(CORE_SOURCES
    src/core/DeviceProfile.cpp
    src/core/ReDroidController.cpp
)

# Common utilities
set(COMMON_SOURCES
    src/common/Logger.cpp
)

# =============================================================================
# Main Executable
# =============================================================================

add_executable(VirtualPhonePro
    src/main.cpp
    ${CORE_SOURCES}
    ${COMMON_SOURCES}
)

# Create precompiled header support
target_precompile_headers(VirtualPhonePro PRIVATE
    <QtCore/QtCore>
    <QtWidgets/QtWidgets>
    <QCoreApplication>
    <QDebug>
)

# =============================================================================
# Qt6 Link Libraries
# =============================================================================

target_link_libraries(VirtualPhonePro PRIVATE
    Qt6::Core
    Qt6::Network
    Qt6::Process
    Qt6::Widgets
    Qt6::Gui
    Qt6::Xml
)

# Windows-specific libraries
if(PLATFORM_WINDOWS)
    target_link_libraries(VirtualPhonePro PRIVATE
        ws2_32
        iphlpapi
        userenv
        advapi32
    )
endif()

# =============================================================================
# Windows Runtime Library
# =============================================================================

if(MSVC)
    # Use static CRT for better portability
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# =============================================================================
# Installation Rules
# =============================================================================

# Install executable
install(TARGETS VirtualPhonePro
    RUNTIME DESTINATION bin
    COMPONENT Runtime
)

# Install platform-tools (if bundled)
install(DIRECTORY ${CMAKE_SOURCE_DIR}/externals/platform-tools/
    DESTINATION bin/platform-tools
    COMPONENT Runtime
    OPTIONAL
)

# Install configuration directory
install(DIRECTORY ${CMAKE_SOURCE_DIR}/configs/
    DESTINATION etc/VirtualPhonePro/configs
    COMPONENT Config
)

# Install documentation
install(DIRECTORY ${CMAKE_SOURCE_DIR}/docs/
    DESTINATION share/doc/VirtualPhonePro
    COMPONENT Docs
)

# =============================================================================
# Post-Build: Copy platform-tools
# =============================================================================

if(EXISTS ${CMAKE_SOURCE_DIR}/externals/platform-tools/adb.exe)
    add_custom_command(TARGET VirtualPhonePro POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/externals/platform-tools"
            "$<TARGET_FILE_DIR:VirtualPhonePro>/platform-tools"
        COMMENT "Copying platform-tools to output directory"
    )
endif()

# =============================================================================
# Qt6 Deployment (windeployqt on Windows)
# =============================================================================

if(WIN32 AND EXISTS "${QT_PREFIX_DIR}/bin/windeployqt.exe")
    add_custom_command(TARGET VirtualPhonePro POST_BUILD
        COMMAND "${QT_PREFIX_DIR}/bin/windeployqt.exe"
            "$<TARGET_FILE:VirtualPhonePro>"
            --no-translations
            --no-system-trimpaths
            --no-virtualkeyboard
            --no-sql
            --no-multimedia
            --no-opengl
            --no-gamepad
            --no-harfbuzz
            --dir "$<TARGET_FILE_DIR:VirtualPhonePro>/qt"
        COMMENT "Deploying Qt runtime dependencies"
    )
endif()

# =============================================================================
# Testing
# =============================================================================

enable_testing()

add_executable(VirtualPhonePro_UnitTests
    tests/unit/TestDeviceProfile.cpp
    tests/unit/TestReDroidController.cpp
    ${CORE_SOURCES}
)

target_link_libraries(VirtualPhonePro_UnitTests PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME VirtualPhonePro_Tests COMMAND VirtualPhonePro_UnitTests)

# =============================================================================
# CPack Configuration
# =============================================================================

set(CPACK_PACKAGE_NAME "VirtualPhonePro")
set(CPACK_PACKAGE_VENDOR "VirtualPhonePro Team")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Anti-detect Android Emulator Manager")
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})

set(CPACK_RESOURCE_FILE_README "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

if(WIN32)
    set(CPACK_GENERATOR "ZIP;NSIS")
    
    # NSIS installer configuration
    set(CPACK_NSIS_PACKAGE_NAME "VirtualPhonePro")
    set(CPACK_NSIS_CONTACT "mostakimnasim5@gmail.com")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/mostakimnasim5/VirtualPhonePro")
    set(CPACK_NSIS_ENABLE_UNINSTALLER ON)
    set(CPACK_NSIS_MENU_LINKS 
        "https://github.com/mostakimnasim5/VirtualPhonePro" "VirtualPhonePro on GitHub"
    )
endif()

include(CPack)

# =============================================================================
# Summary
# =============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "  VirtualPhonePro Build Configuration")
message(STATUS "========================================")
message(STATUS "  Version: ${PROJECT_VERSION}")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "  Qt6 Version: ${Qt6_VERSION}")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "========================================")
message(STATUS "")
