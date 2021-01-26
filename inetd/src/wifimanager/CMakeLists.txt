cmake_minimum_required(VERSION 3.10)

project(wifimanager)
add_library(wifimanager SHARED network_manager.c udhcpc_thread.c wifi.c wifi_event.c wifimanager.c wifi_state_machine.c wpa_supplicant_conf.c)
include_directories(${MINIGUI_INCLUDE_DIRS} ${PROJECT_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR}/include)
target_link_libraries(wifimanager ${MINIGUI_LIBRARIES} wpa_client)
