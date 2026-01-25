cmake_minimum_required(VERSION 3.10)
project(WebCrawler)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(CURL REQUIRED)
find_package(Threads REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Source files - using absolute paths for safety
set(SOURCES
    "${CMAKE_SOURCE_DIR}/src/downloader.cpp"
    "${CMAKE_SOURCE_DIR}/src/parser.cpp"
    "${CMAKE_SOURCE_DIR}/src/url_frontier.cpp"
    "${CMAKE_SOURCE_DIR}/src/storage_manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/thread_manager.cpp"
    "${CMAKE_SOURCE_DIR}/src/utils.cpp"
    "${CMAKE_SOURCE_DIR}/src/main.cpp"
)

# Verify source files exist
foreach(SOURCE_FILE ${SOURCES})
    if(NOT EXISTS "${SOURCE_FILE}")
        message(FATAL_ERROR "Source file not found: ${SOURCE_FILE}")
    endif()
endforeach()

# Create executable
add_executable(crawler ${SOURCES})

# Link libraries
target_link_libraries(crawler PRIVATE 
    ${CURL_LIBRARIES}
    Threads::Threads
)

# Include curl headers
target_include_directories(crawler PRIVATE ${CURL_INCLUDE_DIRS})

# Compiler flags
if(UNIX)
    target_compile_options(crawler PRIVATE -Wall -Wextra -O2)
endif()

# Custom clean target
add_custom_target(clean-all
    COMMAND ${CMAKE_MAKE_PROGRAM} clean
    COMMAND rm -f ${CMAKE_BINARY_DIR}/crawler
    COMMAND rm -f ${CMAKE_BINARY_DIR}/CMakeCache.txt
    COMMAND rm -rf ${CMAKE_BINARY_DIR}/CMakeFiles
    COMMAND rm -f ${CMAKE_BINARY_DIR}/cmake_install.cmake
    COMMAND rm -f ${CMAKE_BINARY_DIR}/Makefile
    COMMAND echo "Clean complete - all build artifacts removed"
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Print configuration
message(STATUS "=== Build Configuration ===")
message(STATUS "Source dir: ${CMAKE_SOURCE_DIR}")
message(STATUS "Build dir: ${CMAKE_BINARY_DIR}")
message(STATUS "Include dir: ${CMAKE_SOURCE_DIR}/include")
message(STATUS "CURL version: ${CURL_VERSION_STRING}")
message(STATUS "CURL libraries: ${CURL_LIBRARIES}")
message(STATUS "Source files:")
foreach(SRC ${SOURCES})
    message(STATUS "  - ${SRC}")
endforeach()
message(STATUS "=== Available Targets ===")
message(STATUS "  make             - Build crawler")
message(STATUS "  make clean       - Remove .o object files")
message(STATUS "  make clean-all   - Remove everything (executable + CMake files)")
message(STATUS "=========================")