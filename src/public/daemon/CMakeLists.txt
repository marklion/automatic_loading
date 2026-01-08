cmake_minimum_required(VERSION 3.10)
project(init_daemon)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ MAIN_SRC)
aux_source_directory(../gen_code/cpp MAIN_SRC)

find_library(AL_UTILS_LIB libal_utils.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

add_executable(init_daemon ${MAIN_SRC})
target_link_libraries(init_daemon ${AL_UTILS_LIB} thrift)