cmake_minimum_required(VERSION 2.8.7)
if(POLICY CMP0046)
  cmake_policy(SET CMP0046 NEW)
endif()
if(POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()

project(FACE C CXX)


set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# --[ Options
option(RELEASE "Build type" ON)
option(build_tests "build unit test" ON)

if(RELEASE)
  set(CMAKE_BUILD_TYPE RELEASE)
else()
  set(CMAKE_BUILD_TYPE DEBUG)
endif()

set(CMAKE_CXX_FLAGS "-fPIC -Wall -Werror -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -DNDEBUG -O3")

set(module_list "")

list(APPEND module_list feature_extract)

# add head file search path

foreach(module ${module_list})
  include_directories(${PROJECT_SOURCE_DIR}/modules/${module}/include)
  file(GLOB_RECURSE module_src ${PROJECT_SOURCE_DIR}/modules/${module}/src/*.cpp)
  list(APPEND srcs ${module_src})
endforeach()


list(APPEND srcs ${PROJECT_SOURCE_DIR}/modules/main.cpp)

add_executable(face ${srcs})
