cmake_minimum_required(VERSION 2.8.7)
if(POLICY CMP0046)
  cmake_policy(SET CMP0046 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

project(FACE C CXX)


set(OpenCV_FIND_QUIETLY true)
     #find_package(OpenCV REQUIRED) //this will include opencv_ts which result in crash on centos
#find_package(OpenCV OPTIONAL_COMPONENTS imgcodecs)
#find_package(OpenCV OPTIONAL_COMPONENTS videoio)
#set(optional_opencv_libs ${OpenCV_LIBS})
find_package(OpenCV REQUIRED)
#if(OpenCV_FOUND)
#if(imgcodecs_FOUND)
#  list(APPEND OpenCV_LIBS optional_opencv_libs)
#endif()


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# --[ Options
option(RELEASE "Build type" ON)
option(build_tests "build unit test" ON)

if(RELEASE)
  set(CMAKE_BUILD_TYPE RELEASE)
else()
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-fPIC -lpthread -Wall -Werror -std=c++11 ")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -DNDEBUG -O3")

set(module_list "")

list(APPEND  module_list feature_extract decoder show)
# add head file search path

find_package(OpenCV REQUIRED)
if(OpenCV_FOUND)
  message("Opencv found!")
  include_directories(${OpenCV_INCLUDE_DIRS})
endif()

find_package(Threads)

foreach(module ${module_list})
  include_directories(${PROJECT_SOURCE_DIR}/modules/${module}/include)
  file(GLOB_RECURSE module_src ${PROJECT_SOURCE_DIR}/modules/${module}/src/*.cpp)
  list(APPEND srcs ${module_src})
endforeach()

include_directories(${PROJECT_SOURCE_DIR}/kernel/include)
include_directories(${PROJECT_SOURCE_DIR}/kernel/utils)
include_directories(${PROJECT_SOURCE_DIR}/3rdparty/)
include_directories(${PROJECT_SOURCE_DIR}/modules/)

file(GLOB_RECURSE kernels ${PROJECT_SOURCE_DIR}/kernel$/src/*.cpp)
list(APPEND srcs ${module_src})


set(OpenCV_LIBS opencv_core opencv_imgproc opencv_highgui opencv_features2d opencv_flann)

list(APPEND srcs ${PROJECT_SOURCE_DIR}/modules/main.cpp)

add_executable(face ${srcs})
target_link_libraries(face ${OpenCV_LIBS} ${CMAKE_THREAD_LIBS_INIT} face_kernel)

add_subdirectory(${PROJECT_SOURCE_DIR}/kernel)
#add_subdirectory(${PROJECT_SOURCE_DIR}/modules)
