cmake_minimum_required(VERSION 3.10)

project(wifi)
add_library(wifi SHARED wifi.c)
include_directories(${MINIGUI_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIRS})
target_link_libraries(wifi ${MINIGUI_LIBRARIES} hibus)
