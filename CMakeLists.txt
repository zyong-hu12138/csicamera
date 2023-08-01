cmake_minimum_required(VERSION 3.12)
project (csicamera)
set(CMAKE_CXX_STANDARD 11)

add_library(csicamera SHARED csicamera.cpp)

target_include_directories(csicamera PUBLIC 
/usr/include/gstreamer-1.0/
/usr/include/glib-2.0/
/usr/lib/aarch64-linux-gnu/glib-2.0/include/)

target_link_libraries(csicamera
    gstreamer-1.0
    gobject-2.0
    glib-2.0
    gstbase-1.0
    gstapp-1.0
)

add_executable(main main.cpp)
target_include_directories(main PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(main csicamera)
