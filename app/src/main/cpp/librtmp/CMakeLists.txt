cmake_minimum_required(VERSION 3.4.1)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DNO_CRYPTO")

add_library( # 编译的库的名称是 rtmp
             rtmp

             # 编译的 rtmp 库是静态库
             STATIC

             # rtmp 库的源文件
             amf.c
             hashswf.c
             log.c
             parseurl.c
             rtmp.c )