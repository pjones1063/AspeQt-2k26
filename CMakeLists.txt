cmake_minimum_required(VERSION 3.16)

project(AspeQt VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Enable Qt Automation ---
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --- Find Qt 6 Components ---
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network SerialPort PrintSupport Concurrent)
find_package(ZLIB REQUIRED)

# --- 1. Find ALL source files in src/ ---
file(GLOB_RECURSE PROJECT_SOURCES
    "src/*.cpp"
    "src/*.h"
    "src/*.ui"
)

# --- 2. Filter out files for the wrong OS ---
if(UNIX)
    # On Linux/Mac, remove Windows-specific files
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*win32.*")
elseif(WIN32)
    # On Windows, remove Linux/Posix specific files
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*posix.*")
    list(FILTER PROJECT_SOURCES EXCLUDE REGEX ".*linux.*")
endif()

# --- 3. Add the Executable (Removed MANUAL_FINALIZATION) ---
qt_add_executable(AspeQt
    ${PROJECT_SOURCES}
    "res/atarifiles.qrc"
    "res/icons.qrc"
    "res/documentation.qrc"
    "res/i18n.qrc"
    "res/firmware.qrc"
)

# --- 4. Include Directories ---
target_include_directories(AspeQt PRIVATE
    "${CMAKE_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_BINARY_DIR}"
)

# --- 5. Link Libraries ---
target_link_libraries(AspeQt PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::SerialPort
    Qt6::PrintSupport
    Qt6::Concurrent
    ZLIB::ZLIB
)

if(WIN32)
    set_property(TARGET AspeQt PROPERTY WIN32_EXECUTABLE ON)
endif()

install(TARGETS AspeQt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

# --- Define the Version String for C++ ---
target_compile_definitions(AspeQt PRIVATE
    VERSION="${PROJECT_VERSION}"
)

# --- Copy Resources to Build Directory ---
# We list every folder you want to copy here
file(GLOB_RECURSE RESOURCES_TO_COPY
    "${CMAKE_SOURCE_DIR}/res/help/*"
    "${CMAKE_SOURCE_DIR}/res/images/*"
    "${CMAKE_SOURCE_DIR}/res/silk-icons/*"
    "${CMAKE_SOURCE_DIR}/res/oxygen-icons/*"
    "${CMAKE_SOURCE_DIR}/res/main-icon/*"
    "${CMAKE_SOURCE_DIR}/res/other-icons/*"
)

foreach(resource ${RESOURCES_TO_COPY})
    # Get the relative path (e.g., "silk-icons/disk.png")
    file(RELATIVE_PATH relative_path "${CMAKE_SOURCE_DIR}" "${resource}")

    # Define destination in the build folder
    set(destination "${CMAKE_CURRENT_BINARY_DIR}/${relative_path}")

    # Create the folder structure
    get_filename_component(dir "${destination}" DIRECTORY)
    file(MAKE_DIRECTORY "${dir}")

    # Copy the file
    configure_file("${resource}" "${destination}" COPYONLY)
endforeach()
