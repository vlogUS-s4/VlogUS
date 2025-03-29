cmake_minimum_required(VERSION 3.10)
project(face_tracking LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/src/face_tracking.cpp")
    message(FATAL_ERROR "Source file src/face_tracking.cpp not found!")
endif()

add_library(face_tracking SHARED src/face_tracking.cpp)

find_package(OpenCV REQUIRED)
if(NOT OpenCV_FOUND)
    message(FATAL_ERROR "OpenCV not found. Please set OpenCV_DIR to the OpenCV build/install directory.")
endif()
message(STATUS "OpenCV found: ${OpenCV_VERSION}")
message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")

target_include_directories(face_tracking PRIVATE ${OpenCV_INCLUDE_DIRS})
target_link_libraries(face_tracking PRIVATE ${OpenCV_LIBS})

# Set output directories for all build types
set_target_properties(face_tracking PROPERTIES
    # For .dll (Windows) or .so (Linux)
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    # For .a (import library on MinGW)
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
    # For .so (if on Linux, but also ensures consistency)
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# Ensure MinGW prepends "lib" to the library name (optional, for consistency)
if(MINGW)
    set_target_properties(face_tracking PROPERTIES PREFIX "lib")
endif()

if(WIN32)
    target_compile_definitions(face_tracking PRIVATE FACE_TRACKING_EXPORTS)
endif()

install(TARGETS face_tracking
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    ARCHIVE DESTINATION lib
)