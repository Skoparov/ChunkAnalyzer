cmake_minimum_required(VERSION 2.6)
cmake_policy (SET CMP0020 NEW)

SET(CMAKE_BUILD_TYPE Debug)

project ("FileAnalyzer")
message ( STATUS "FileAnalyzer" )

set(CMAKE_AUTOMOC ON)

set (CMAKE_FIND_ROOT_PATH ${ENV_PATH})
set (CMAKE_PREFIX_PATH ${ENV_PATH})
set (CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/BIN/)

include_directories("./")
include_directories("./SRC/Loader")
include_directories("./SRC/Analyzer")
include_directories("${CMAKE_BINARY_DIR}")	

find_package(Qt5 COMPONENTS Core Network)
#find_package (Qt4 4.8.4 REQUIRED QtCore QtNetwork)
#include (${QT_USE_FILE})

add_subdirectory(SRC)
