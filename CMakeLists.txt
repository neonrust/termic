# https://gitlab.com/CLIUtils/modern-cmake/-/tree/master/examples/extended-project

cmake_minimum_required(VERSION 3.11)

project(termic
	VERSION 0.1
	DESCRIPTION "TERMinal Interface (using) Cells"
	LANGUAGES CXX
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# library
add_subdirectory(src)

# demo application(s)
add_subdirectory(examples/demo1)


if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
	include(CTest)
endif()
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
	add_subdirectory(tests)
endif()
