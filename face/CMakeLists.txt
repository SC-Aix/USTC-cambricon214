cmake_minimum_required(VERSION 2.8.7)
if(POLICY CMP0046)
  cmake_policy(SET CMP0046 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

project(FACE C CXX)


set(OpenCV_FIND_QUIETLY true)
if(NOT DEFINED ENV{NEUWARE_HOME})
  set(ENV{NEUWARE_HOME} /usr/local/neuware)
endif()

#if(MLU_PLATFORM STREQUAL MLU270 OR MLU_PLATFORM STREQUAL MLU220_SOC OR MLU_PLATFORM STREQUAL MLU220)
list(APPEND CN_LIBS easydk cnis cnrt cncodec cndrv)
#else()
 #uiendif()

include_directories("$ENV{NEUWARE_HOME}/include")
link_directories("$ENV{NEUWARE_HOME}/lib64")



set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# --[ Options
option(RELEASE "Build type" ON)
option(build_tests "build unit test" ON)

if(RELEASE)
  set(CMAKE_BUILD_TYPE RELEASE)
else()
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-fPIC -lpthread -Wall -Werror -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -DNDEBUG -O3")

set(module_list "")

list(APPEND  module_list utils warpffine decoder show inference opencvdetect opencvextract)
# add head file search path

#set(OpenCV_DIR ${CMAKE_CURRENT_SOURCE_DIR}/../../opencv/opencv_install)
# include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../opencv/opencv_install/include)
# link_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../opencv/opencv_install/lib)
find_package(OpenCV REQUIRED core imgproc highgui features2d objdetect xobjdetect face
              PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../../opencv/opencv_install
              NO_DEFAULT_PATH)
#find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
   message("Opencv found! --- ${OpenCV_INCLUDE_DIRS}")
   include_directories(${OpenCV_INCLUDE_DIRS})
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
find_package(FFmpeg REQUIRED)
if (FFMPEG_FOUND)
  include_directories(${FFMPEG_INCLUDE_DIR})
  message(STATUS "FFmpeg Found, include: ${FFMPEG_INCLUDE_DIR}")
  message(STATUS "FFmpeg Found, libraries: ${FFMPEG_LIBRARIES}")
else (FFMPEG_FOUND)
  message(FATAL_ERROR "Can not find FFmpeg libs!")
endif (FFMPEG_FOUND)


find_package(Threads)

foreach(module ${module_list})
  include_directories(${PROJECT_SOURCE_DIR}/modules/${module}/include)
  message("${PROJECT_SOURCE_DIR}/modules/${module}/include\n")
  file(GLOB_RECURSE module_src ${PROJECT_SOURCE_DIR}/modules/${module}/src/*.cpp)
  list(APPEND srcs ${module_src})
endforeach()

# inference postprocess
include_directories(${PROJECT_SOURCE_DIR}/modules/inference/postproc/)
file(GLOB_RECURSE module_src ${PROJECT_SOURCE_DIR}/modules/inference/postproc/*.cpp)
list(APPEND srcs ${module_src})

# inference postprocess
include_directories(${PROJECT_SOURCE_DIR}/modules/inference/preproc/)
file(GLOB_RECURSE module_src ${PROJECT_SOURCE_DIR}/modules/inference/preproc/*.cpp)
list(APPEND srcs ${module_src})

list(APPEND srcs ${PROJECT_SOURCE_DIR}/modules/frame.cpp)

include_directories(${PROJECT_SOURCE_DIR}/kernel/include)
include_directories(${PROJECT_SOURCE_DIR}/kernel/utils)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/easydk/include)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/easydk/infer_server/include)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/easydk/infer_server/contrib)
include_directories(${PROJECT_SOURCE_DIR}/modules/)

file(GLOB_RECURSE kernels ${PROJECT_SOURCE_DIR}/kernel/src/*.cpp)
list(APPEND srcs ${module_src})


set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_features2d opencv_flann opencv_objdetect opencv_face)

list(APPEND srcs ${PROJECT_SOURCE_DIR}/modules/main.cpp)

link_directories(${PROJECT_SOURCE_DIR}/3rdparty/easydk/lib/)

add_executable(face ${srcs})

target_link_libraries(face ${OpenCV_LIBS} ${OpenCV_LIBRARY_DIRS} ${FFMPEG_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT} face_kernel  ${CN_LIBS})
#target_link_libraries(face ${PROJECT_SOURCE_DIR}/3rdparty/easydk/lib/libeasydk.so)
message("--${OpenCV_LIBS}--------${OpenCV_INCLUDE_DIRS}  ----${OpenCV_LIBRARY_DIRS}")
add_subdirectory(${PROJECT_SOURCE_DIR}/kernel)
#add_subdirectory(${PROJECT_SOURCE_DIR}/3rdparty/easydk)
#add_subdirectory(${PROJECT_SOURCE_DIR}/modules)
