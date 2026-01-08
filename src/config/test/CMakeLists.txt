cmake_minimum_required(VERSION 3.10)
project(config_test)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ TEST_SRC)
find_library(CONFIG_LIB libconfig_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

add_executable(config_test ${TEST_SRC})
target_link_libraries(config_test ${CONFIG_LIB})