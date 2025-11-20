cmake_minimum_required(VERSION 3.15)
project(UIBro VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows-only project
if(NOT WIN32)
    message(FATAL_ERROR "UIBro requires Windows to build")
endif()

# WTL Configuration
set(WTL_INCLUDE_DIR "" CACHE PATH "Path to WTL include directory")

if(NOT WTL_INCLUDE_DIR)
    # Try to find WTL in common locations
    find_path(WTL_INCLUDE_DIR
        NAMES atlapp.h
        PATHS
            "C:/WTL/Include"
            "C:/Program Files (x86)/WTL/Include"
            "C:/Program Files/WTL/Include"
            "$ENV{ProgramFiles}/WTL/Include"
            "$ENV{ProgramFiles\(x86\)}/WTL/Include"
        DOC "WTL Include Directory"
    )
endif()

if(NOT WTL_INCLUDE_DIR)
    message(FATAL_ERROR 
        "WTL not found. Please download WTL from:\n"
        "https://sourceforge.net/projects/wtl/\n"
        "And set WTL_INCLUDE_DIR to the Include directory, e.g.:\n"
        "cmake -DWTL_INCLUDE_DIR=C:/WTL/Include .."
    )
else()
    message(STATUS "WTL found: ${WTL_INCLUDE_DIR}")
endif()

# ATL Configuration (usually comes with Visual Studio)
if(MSVC)
    # ATL is included with Visual Studio
    message(STATUS "Using MSVC - ATL should be available")
else()
    message(WARNING "Non-MSVC compiler detected. ATL/WTL may not be available.")
endif()

# Compiler-specific settings
if(MSVC)
    # MSVC-specific flags
    add_compile_options(
        /W4                 # Warning level 4
        /permissive-        # Standards conformance
        /Zc:__cplusplus     # Correct __cplusplus macro
        /EHsc               # Exception handling
        /MP                 # Multi-processor compilation
    )
    
    # Linker flags for Windows subsystem
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\"")
    
    # DPI Awareness manifest
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFESTDEPENDENCY:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
else()
    message(FATAL_ERROR "UIBro with WTL requires Visual Studio (MSVC)")
endif()

# Windows definitions
add_definitions(
    -DUNICODE
    -D_UNICODE
    -DWIN32_LEAN_AND_MEAN
    -DNOMINMAX
    -D_WTL_NO_CSTRING
    -D_WTL_NO_WTYPES
    -D_ATL_NO_HOSTING
)

# Main executable
add_executable(UIBro WIN32
    src/main.cpp
)

# Include directories
target_include_directories(UIBro PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${WTL_INCLUDE_DIR}
)

# Link Windows libraries
target_link_libraries(UIBro PRIVATE
    comctl32        # Common Controls
    uxtheme         # Visual Styles
    dwmapi          # Desktop Window Manager
    shell32         # Shell API
    user32          # User Interface
    gdi32           # Graphics Device Interface
)

# Set target properties
set_target_properties(UIBro PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "UIBro"
    WIN32_EXECUTABLE TRUE
)

# Copy UIBro.hpp to include directory for distribution
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UIBro.hpp"
    "${CMAKE_BINARY_DIR}/include/UIBro.hpp"
    COPYONLY
)

# Installation rules
install(TARGETS UIBro
    RUNTIME DESTINATION bin
)

install(FILES 
    "${CMAKE_CURRENT_SOURCE_DIR}/src/UIBro.hpp"
    DESTINATION include
)

# Print build configuration
message(STATUS "")
message(STATUS "UIBro Configuration:")
message(STATUS "  Version:           ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:      ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build Type:        ${CMAKE_BUILD_TYPE}")
message(STATUS "  WTL Include:       ${WTL_INCLUDE_DIR}")
message(STATUS "  Install Prefix:    ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
