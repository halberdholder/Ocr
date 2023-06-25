cmake_minimum_required(VERSION 3.1.0)
set(CMAKE_VERBOSE_MAKEFILE on)

set(PROJECT_NAME ocr)

project(${PROJECT_NAME})

set(CMAKE_CXX_STANDARD 17)

message("CMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR}")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Release")
endif()
message("CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

option(DEBUG_LOG "enable debug log" OFF)
option(TRACE_LOG "enable trace log" OFF)

if (DEBUG_LOG)
  add_definitions(-DDEBUG_LOG)
endif()

if (TRACE_LOG)
  add_definitions(-DTRACE_LOG)
endif()

add_definitions(-DELPP_NO_DEFAULT_LOG_FILE)
add_definitions("-Wall")
add_definitions("-fopenmp")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--rpath=./libs")

#if (WIN32 OR MSVC)
#  set(CMAKE_FIND_LIBRARY_SUFFIXES ".lib")
#elseif (UNIX)
#  set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
#endif()

find_package(OpenCV COMPONENTS
    opencv_videoio
    opencv_core
    opencv_imgcodecs
    opencv_imgproc
    REQUIRED)

if (OpenCV_FOUND)
  message("OpenCV_INCLUDE_DIRS = ${OpenCV_INCLUDE_DIRS}")
  message("OpenCV_LIBRARIES = ${OpenCV_LIBRARIES}")
  message("OpenCV_VERSION = ${OpenCV_VERSION}")

  include_directories(${OpenCV_INCLUDE_DIRS})
else()
  message(FATAL_ERROR "OpenCV not found")
endif()

find_package(Leptonica REQUIRED)
if (Leptonica_FOUND)
  message("Leptonica_LIBRARIES = ${Leptonica_LIBRARIES}")
else()
  message(FATAL_ERROR "Leptonica not found")
endif()

find_package(Tesseract REQUIRED)
if (Tesseract_FOUND)
  message("Tesseract_LIBRARIES = ${Tesseract_LIBRARIES}")
else()
  message(FATAL_ERROR "Leptonica not found")
endif()

include_directories(src)

foreach(curdir main core/impl utils/impl 3rdparty/easyloggingpp)
    file(GLOB_RECURSE cursrcs src/${curdir}/*.cpp)
    list(APPEND SRC_CPPS "${cursrcs}")
endforeach()

add_executable(${PROJECT_NAME} ${SRC_CPPS})

target_link_libraries(${PROJECT_NAME}
    ${OpenCV_LIBRARIES} 
    -Wl,--whole-archive
    ${Leptonica_LIBRARIES} 
    -Wl,--no-whole-archive
    ${Tesseract_LIBRARIES} 
    -lopenjp2
    -lcurl
    -larchive
)

set(CMAKE_INSTALL_PREFIX ${CMAKE_CURRENT_SOURCE_DIR})
set(INSTALL_DIR dist)
install(TARGETS ${PROJECT_NAME} DESTINATION ${INSTALL_DIR})

if ("${CMAKE_SYSTEM_PROCESSOR}" STREQUAL "aarch64")
  set(HOST_LIB_DIR "/usr/lib/aarch64-linux-gnu")
else() # x86_64
  set(HOST_LIB_DIR "/lib/x86_64-linux-gnu")
endif()

set(LIBS_DIR ${INSTALL_DIR}/libs)
add_custom_target(copy
  COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_SOURCE_DIR}/${LIBS_DIR}
  COMMAND ${CMAKE_COMMAND} -E copy ${HOST_LIB_DIR}/libopenjp2.so.7 ${CMAKE_CURRENT_SOURCE_DIR}/${LIBS_DIR}/ || true
  COMMAND ${CMAKE_COMMAND} -E copy ${HOST_LIB_DIR}/libavcodec.so.5? ${CMAKE_CURRENT_SOURCE_DIR}/${LIBS_DIR}/ || true
  COMMAND ${CMAKE_COMMAND} -E copy ${HOST_LIB_DIR}/libavformat.so.5? ${CMAKE_CURRENT_SOURCE_DIR}/${LIBS_DIR}/ || true
  COMMAND ${CMAKE_COMMAND} -E copy ${HOST_LIB_DIR}/libavutil.so.5? ${CMAKE_CURRENT_SOURCE_DIR}/${LIBS_DIR}/ || true
  COMMAND ${CMAKE_COMMAND} -E copy ${HOST_LIB_DIR}/libswscale.so.? ${CMAKE_CURRENT_SOURCE_DIR}/${LIBS_DIR}/ || true
)
