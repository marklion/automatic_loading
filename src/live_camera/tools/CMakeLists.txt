cmake_minimum_required(VERSION 3.10)
project(live_camera_tool)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ MAIN_SRC)

add_executable(hk_tool ${MAIN_SRC})
target_link_libraries(hk_tool pthread hcnetsdk)