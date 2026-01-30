cmake_minimum_required(VERSION 3.10)
project(pid_control_test)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ TEST_SRC)
find_library(PID_CONTROL_LIB libpid_control_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

add_executable(pid_control_test ${TEST_SRC})
target_link_libraries(pid_control_test ${PID_CONTROL_LIB})