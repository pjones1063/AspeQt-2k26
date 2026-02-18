cmake_minimum_required(VERSION 3.16)

project(AspeQt VERSION 1.0.0 LANGUAGES CXX)

include(GNUInstallDirs)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Enable Qt Automation ---
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --- 0. Define Icon Path (MacOS specific) ---
# We define this early so we can use it in the executable source list
set(MACOS_ICON_FILE "res/icons.icns")

# --- 1. Setup ZLIB (Cross-platform Fetch) ---
include(FetchContent)
FetchContent_Declare(
  zlib
  URL https://github.com/madler/zlib/archive/refs/tags/v1.3.1.zip
)
FetchContent_MakeAvailable(zlib)

if(NOT TARGET ZLIB::ZLIB)
    if(TARGET zlibstatic)
        add_library(ZLIB::ZLIB ALIAS zlibstatic)
    else()
        add_library(ZLIB::ZLIB ALIAS zlib)
    endif()
endif()

# --- 2. Find Qt 6 Components ---
# FIX: Added 'Xml' to the list of required components
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network SerialPort PrintSupport Concurrent Xml)

# --- 3. Find ALL source files in src/ ---
file(GLOB_RECURSE PROJECT_SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/*.ui"
)


# --- 4. Filter out files for the wrong OS ---
if(WIN32)
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*posix.*")
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*linux.*")
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*unix.*")
elseif(UNIX)
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*win32.*")
endif()

# --- 5. Add the Executable ---
# NOTE: We include ${MACOS_ICON_FILE} in the source list.
# This ensures CMake is aware of the file and can copy it into the bundle.
qt_add_executable(AspeQt
    ${PROJECT_SOURCES}
    ${MACOS_ICON_FILE}
    "res/atarifiles.qrc"
    "res/icons.qrc"
    "res/documentation.qrc"
    "res/i18n.qrc"
    "res/firmware.qrc"
)

# --- 6. Include Directories ---
target_include_directories(AspeQt PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

# --- 7. Link Libraries ---
target_link_libraries(AspeQt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
    Qt6::PrintSupport
    Qt6::Concurrent
    Qt6::Xml
    ZLIB::ZLIB
)

# REQUIRED FIX: Link Windows Multimedia API for timing functions
if(WIN32)
    target_link_libraries(AspeQt PRIVATE winmm)
    set_property(TARGET AspeQt PROPERTY WIN32_EXECUTABLE ON)
endif()

# --- 8. MacOS Bundle & Icon Settings ---
if(APPLE)
    # A. Mark the .icns file as a resource file so it goes into Contents/Resources
    set_source_files_properties(${MACOS_ICON_FILE} PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
    )

    # B. Set the Bundle properties
    set_target_properties(AspeQt PROPERTIES
        MACOSX_BUNDLE ON
        MACOSX_BUNDLE_GUI_IDENTIFIER "net.13leader.AspeQt"
        MACOSX_BUNDLE_BUNDLE_NAME "AspeQt"
        # C. Tell Info.plist to use the filename "icons.icns"
        MACOSX_BUNDLE_ICON_FILE "icons.icns"
    )
endif()

# --- 9. Install Definitions ---
install(TARGETS AspeQt
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)


target_compile_definitions(AspeQt PRIVATE
    VERSION="${PROJECT_VERSION}"
)

# --- 10. Copy Resources to Build Directory ---
# This ensures non-embedded resources (like help files) are available near the binary
file(GLOB_RECURSE RESOURCES_TO_COPY
    "${CMAKE_SOURCE_DIR}/res/help/*"
    "${CMAKE_SOURCE_DIR}/res/images/*"
    "${CMAKE_SOURCE_DIR}/res/silk-icons/*"
    "${CMAKE_SOURCE_DIR}/res/oxygen-icons/*"
    "${CMAKE_SOURCE_DIR}/res/main-icon/*"
    "${CMAKE_SOURCE_DIR}/res/other-icons/*"
)

foreach(resource ${RESOURCES_TO_COPY})
    file(RELATIVE_PATH relative_path "${CMAKE_SOURCE_DIR}" "${resource}")
    set(destination "${CMAKE_CURRENT_BINARY_DIR}/${relative_path}")
    get_filename_component(dir "${destination}" DIRECTORY)
    file(MAKE_DIRECTORY "${dir}")
    configure_file("${resource}" "${destination}" COPYONLY)
endforeach()

# --- CPack Configuration ---
set(CPACK_PACKAGE_NAME "AspeQt-2K26")
set(CPACK_PACKAGE_VENDOR "Paul Jones")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Atari Serial Peripheral Emulator")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "AspeQt-2K26")

# Debian specific settings
if(UNIX AND NOT APPLE)
    set(CPACK_GENERATOR "DEB")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Paul Jones") # Required for .deb
    set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
    set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
endif()

include(CPack)
