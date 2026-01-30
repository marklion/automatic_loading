cmake_minimum_required(VERSION 3.10)
project(pid_control_lib)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ LIB_SRC)
find_library(AL_UTILS_LIB libal_utils.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

add_library(pid_control_lib SHARED ${LIB_SRC})
target_link_libraries(pid_control_lib ${AL_UTILS_LIB})