project(xmlrcsd)
cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
add_executable(${PROJECT_NAME} hiredis/async.c hiredis/dict.c hiredis/hiredis.c  hiredis/net.c  hiredis/sds.c ${SRC_LIST})

