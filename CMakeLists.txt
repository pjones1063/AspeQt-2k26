cmake_minimum_required(VERSION 3.16)

project(AspeQt LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# Automatically generate meta-object code, UI code, and resources
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)


# --- 1. Find Qt 6 Packages ---
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Network 
    Widgets 
    PrintSupport 
    SerialPort
)

find_package(ZLIB REQUIRED)


# --- 2. Define Sources (Auto-Discovery) ---
file(GLOB SOURCES "*.cpp" "*.h" "*.ui" "*.qrc")

# FILTERING: Remove the files that don't belong to your OS
if(UNIX)
    # On Linux, remove the Windows serial port files
    list(FILTER SOURCES EXCLUDE REGEX "serialport-win32")
elseif(WIN32)
    # On Windows, remove the Unix serial port files
    list(FILTER SOURCES EXCLUDE REGEX "serialport-unix")
endif()

# Debug: Check the output to make sure win32 files are gone
message(STATUS "Final Source List: ${SOURCES}")


# --- 3. Create Executable ---
# We removed "manual-finalization" so Qt handles everything automatically
qt_add_executable(AspeQt ${SOURCES})

# --- 4. Configuration & Linking ---
target_compile_definitions(AspeQt PRIVATE VERSION="r2k26")

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    target_compile_definitions(AspeQt PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

target_include_directories(AspeQt PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(AspeQt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    Qt6::Widgets
    Qt6::PrintSupport
    Qt6::SerialPort
    ZLIB::ZLIB
)

if(WIN32)
    target_link_libraries(AspeQt PRIVATE winmm)
endif()

# Note: We removed qt_finalize_executable() because qt_add_executable() 
# does it automatically now.
