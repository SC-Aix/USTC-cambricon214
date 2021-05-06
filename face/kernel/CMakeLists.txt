set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib/)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

include_directories(${PROJECT_SOURCE_DIR}/3rdparty/rapidjson/include)

set(kernel_list kernel)
foreach(module ${kernel_list})
  include_directories(${PROJECT_SOURCE_DIR}/${module}/include)
  include_directories(${PROJECT_SOURCE_DIR}/${module}/utils)
  file(GLOB_RECURSE kernel_src ${PROJECT_SOURCE_DIR}/${module}/src/*.cpp)
  file(GLOB_RECURSE kernel_src ${PROJECT_SOURCE_DIR}/${module}/*.cpp)
  message("-------------------${kernel_src}")
  list(APPEND k_srcs ${kernel_src})
endforeach()

add_library(face_kernel SHARED ${k_srcs})

set(SOURCE_LINKER_LIBS dl pthread)
#install(TARGERTS face_kernel LIBRARY DESTINATION lib) # do what?
target_link_libraries(face_kernel ${SOURCE_LINKER_LIBS})


