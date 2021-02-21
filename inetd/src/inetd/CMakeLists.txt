cmake_minimum_required(VERSION 3.10)
project(hiinetd)

FIND_PATH(hibox_inc_dir hibox/utils.h)
INCLUDE_DIRECTORIES(${hibox_inc_dir})

FIND_LIBRARY(libhibox NAMES hibox)

file(COPY "inetd.cfg" DESTINATION ${PROJECT_SOURCE_DIR}/../../bin/)

add_executable(hiinetd main.c wifi.c ethernet.c mobile.c common.c tools.c)
include_directories(${MINIGUI_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIRS})
target_link_libraries(hiinetd ${MINIGUI_LIBRARIES} ${libhibox} hibus dl)
install(TARGETS hiinetd DESTINATION bin)
