cmake_minimum_required(VERSION 3.10)
project(MyTinyHttpd C)

set(CMAKE_C_STANDARD 99)

# 查找库
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBEVENT REQUIRED libevent)
pkg_check_modules(MYSQL REQUIRED mysqlclient)

# 头文件目录
include_directories(
    include
    ${LIBEVENT_INCLUDE_DIRS}
    ${MYSQL_INCLUDE_DIRS}
)

# 自动收集 src 目录中的所有源文件（包含新增文件）
file(GLOB SRC
    CONFIGURE_DEPENDS
    src/*.c
)

# 可执行文件
add_executable(MyTinyHttpd ${SRC})

# ---- 添加 cJSON 静态库 ----
add_library(cjson STATIC third_party/cJSON/cJSON.c)
target_include_directories(cjson PUBLIC third_party/cJSON)

# ---- 统一使用 keyword signature ----
target_link_libraries(MyTinyHttpd PRIVATE
    ${LIBEVENT_LIBRARIES}
    ${MYSQL_LIBRARIES}
    cjson
)

