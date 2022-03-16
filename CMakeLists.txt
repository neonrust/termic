# https://gitlab.com/CLIUtils/modern-cmake/-/tree/master/examples/extended-project

cmake_minimum_required(VERSION 3.11)

project(termic
	VERSION 0.1
	DESCRIPTION "TERMinal Interface (using) Cells"
	LANGUAGES CXX
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

option(TERMIC_BUILD_EXAMPLES "Build examples.   Default=ON" ON)
option(TERMIC_BUILD_TESTS    "Build tests.      Default=ON" ON)

# library
add_subdirectory(src)

if(TERMIC_BUILD_EXAMPLES)
	# demo application(s)
	add_subdirectory(examples/demo1)
endif()

if(TERMIC_BUILD_TESTS)
	if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
		include(CTest)
	endif()
	if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME AND BUILD_TESTING)
		add_subdirectory(tests)
	endif()
endif()
