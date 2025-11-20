cmake_minimum_required(VERSION 3.15)
project(UIBro VERSION 3.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DWIN32_LEAN_AND_MEAN -DNOMINMAX)
    
    # Enable DPI awareness
    if(MSVC)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFESTUAC:\"level='asInvoker' uiAccess='false'\"")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:WINDOWS")
        add_compile_options(/permissive- /W4)
    endif()
endif()

# UIBro executable
add_executable(UIBro WIN32
    src/main.cpp
)

target_include_directories(UIBro PRIVATE src)

# Link Windows libraries
target_link_libraries(UIBro PRIVATE
    comctl32
    uxtheme
    dwmapi
    shell32
    user32
    gdi32
    runtimeobject
    windowsapp
)

# Set output directory
set_target_properties(UIBro PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "uibro"
)

# Installation
install(TARGETS UIBro RUNTIME DESTINATION bin)
