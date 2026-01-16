cmake_minimum_required(VERSION 3.10)
project(ad_cli)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ MAIN_SRC)
find_library(AL_UTILS_LIB libal_utils.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(LOG_LIB liblog_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(MODBUS_IO_LIB libmodbus_io_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(STATE_MACHINE_LIB libstate_machine_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(LIDAR_LIB liblidar_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(XLRD_LIB libxlrd_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(LIVE_CAMERA_LIB liblive_camera_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

add_executable(ad_cli ${MAIN_SRC})
target_link_libraries(ad_cli ${AL_UTILS_LIB} ${LOG_LIB} ${XLRD_LIB} ${MODBUS_IO_LIB} ${STATE_MACHINE_LIB} ${LIDAR_LIB} ${LIVE_CAMERA_LIB} thrift pam pam_misc)