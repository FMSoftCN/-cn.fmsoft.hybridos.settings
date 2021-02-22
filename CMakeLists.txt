cmake_minimum_required(VERSION 3.10)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(MiniGUI 5.0.2 REQUIRED)

set(MINIGUI_LIBRARIES ${MiniGUI_LIBRARIES})
set(MINIGUI_INCLUDE_DIRS ${MiniGUI_INCLUDE_DIRS})

OPTION(PLATFORM_R818 "Compiled for platform R818" OFF)

if(PLATFORM_R818)
  MESSAGE(STATUS "Compiled for platform R818")
  ADD_DEFINITIONS(-DPLATFORM_R818)
endif()

# sub-directory
add_subdirectory(inetd)
