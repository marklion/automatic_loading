cmake_minimum_required(VERSION 3.10)
project(plate_gate_lib)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ LIB_SRC)
aux_source_directory(../gen_code/cpp RPC_SRC)

add_library(plate_gate_lib SHARED ${LIB_SRC} ${RPC_SRC})
find_library(AL_UTILS_LIB libal_utils.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

target_link_libraries(plate_gate_lib ${AL_UTILS_LIB})