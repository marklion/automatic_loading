cmake_minimum_required(VERSION 3.10)
project(al_utils)

SET(LIBRARY_OUTPUT_PATH ${CUR_BUILD_PATH}/lib)
SET(EXECUTABLE_OUTPUT_PATH ${CUR_BUILD_PATH}/bin)
add_compile_options(-std=gnu++14 -g)

aux_source_directory(./ LIB_SRC)
# also pick up detail implementation files (e.g., linuxkeyboard.cpp)
aux_source_directory(./detail LIB_SRC)
aux_source_directory(../gen_code/cpp LIB_SRC)

add_library(al_utils SHARED ${LIB_SRC})
target_link_libraries(al_utils thrift modbus)