# https://gitlab.com/CLIUtils/modern-cmake/-/tree/master/examples/extended-project

cmake_minimum_required(VERSION 3.5)

project(termic
	VERSION 0.1
	DESCRIPTION "TERMinal Interface (using) Cells"
	LANGUAGES CXX
)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

#set(CXX g++)
#set(CC gcc)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wextra -Wall -Wpedantic -Wconversion -Werror -Wno-padded)

set(lib_sources
	src/app.h
	src/app.cpp
	src/event.h
	src/terminal.h
	src/terminal.cpp
	src/cell.h
	src/size.h
	src/samplers.h
	src/samplers.cpp
	src/screen.h
	src/screen.cpp
	src/screen-buffer.h
	src/screen-buffer.cpp
	src/input.h
	src/input.cpp
	src/canvas.h
	src/canvas.cpp
	src/keycodes.h
	src/keycodes.cpp
)

#FetchContent_MakeAvailable(fmtlib)

include_directories(termic extern/TheWhisp-signals/)

add_library(termic STATIC ${lib_sources})

add_subdirectory(examples/demo1)

# We need this directory, and users of our library will need it too
target_include_directories(termic PUBLIC src)
