cmake_minimum_required(VERSION 3.10)
project(log_daemon)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ MAIN_SRC)
aux_source_directory(../gen_code/cpp RPC_SRC)
find_library(LOG_LIB liblog_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(AL_UTILS_LIB libal_utils.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(CONFIG_LIB libconfig_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

add_executable(log_daemon ${MAIN_SRC} ${RPC_SRC})
target_link_libraries(log_daemon ${LOG_LIB} ${AL_UTILS_LIB} ${CONFIG_LIB} thrift)