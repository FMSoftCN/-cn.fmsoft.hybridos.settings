cmake_minimum_required(VERSION 3.10)

project(inetd)
set(PROJECT_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/include)

link_directories("${PROJECT_SOURCE_DIR}/lib")

# copy the necessary resource files to /bin/res
file(MAKE_DIRECTORY ${PROJECT_SOURCE_DIR}/bin)

# set output path
SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# set debug or release
#SET(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
#ADD_DEFINITIONS(-O0 -g -Wall)
# -Wunused-variable -Wunused-parameter -Wunused-function -Wunused
# -Wno-deprecated -Woverloaded-virtual -Wwrite-strings
# -D__WUR= -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DTIXML_USE_STL
#)


# set sub-directory
add_subdirectory(src)
