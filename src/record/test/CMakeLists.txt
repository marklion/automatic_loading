cmake_minimum_required(VERSION 3.10)
project(record_test)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ TEST_SRC)
find_library(RECORD_LIB librecord_lib.so ${CUR_BUILD_PATH}/lib ${PRJ_INTERNAL_BUILD}/lib)

add_executable(record_test ${TEST_SRC})
target_link_libraries(record_test ${RECORD_LIB} thrift)