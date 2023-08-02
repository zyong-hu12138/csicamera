cmake_minimum_required(VERSION 3.12)
project(csicamera)
set(CMAKE_CXX_STANDARD 11)

# 添加共享库 csicamera
add_library(csicamera SHARED csicamera.cpp semmap.cpp)
include_directories(csicamera PUBLIC  ${CMAKE_CURRENT_SOURCE_DIR} 
    /usr/include/gstreamer-1.0/
    /usr/include/glib-2.0/
    /usr/lib/aarch64-linux-gnu/glib-2.0/include/
) # 添加当前源代码目录到头文件搜索路径
link_libraries(csicamera
    gstreamer-1.0
    gobject-2.0
    glib-2.0
    gstbase-1.0
    gstapp-1.0
)

# 添加可执行文件 main
add_executable(main main.cpp)
target_link_libraries(main csicamera)
target_include_directories(main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} 
/usr/include/gstreamer-1.0/
/usr/include/glib-2.0/
/usr/lib/aarch64-linux-gnu/glib-2.0/include/
) # 添加当前源代码目录到头文件搜索路径
target_link_libraries(main
gstreamer-1.0
gobject-2.0
glib-2.0
gstbase-1.0
gstapp-1.0
)
