cmake_minimum_required(VERSION 3.10)
project(lidar_daemon)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ MAIN_SRC)
aux_source_directory(../gen_code/cpp RPC_SRC)

find_package(PCL 1.10 REQUIRED)
include_directories(${PCL_INCLUDE_DIRS})
link_directories(${PCL_LIBRARY_DIRS})
add_definitions(${PCL_DEFINITIONS})

add_subdirectory(${PROJECT_SOURCE_DIR}/rs_driver)
find_package(rs_driver REQUIRED)
include_directories(${rs_driver_INCLUDE_DIRS})

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

find_library(LOG_LIB liblog_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(AL_UTILS_LIB libal_utils.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(CONFIG_LIB libconfig_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)
find_library(STATE_MACHINE_LIB libstate_machine_lib.so ${PRJ_INTERNAL_BUILD}/lib ${CUR_BUILD_PATH}/lib)

add_executable(lidar_daemon ${MAIN_SRC} ${RPC_SRC})
target_link_libraries(lidar_daemon ${LOG_LIB} ${AL_UTILS_LIB} ${CONFIG_LIB} ${STATE_MACHINE_LIB} ${rs_driver_LIBRARIES} ${PCL_LIBRARIES} ${VTK_LIBRARIES} thrift)