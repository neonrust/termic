include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(lib_headers
	../include/termic/app.h
	../include/termic/canvas.h
	../include/termic/cell.h
	../include/termic/event.h
	../include/termic/input.h
	../include/termic/keycodes.h
	../include/termic/samplers.h
	../include/termic/screen.h
	../include/termic/screen-buffer.h
	../include/termic/size.h
	../include/termic/terminal.h
	../include/termic/utf8.h
	../include/termic/stopwatch.h
	../include/termic/text.h
	../include/termic/look.h
	../extern/mk-wcwidth/mk-wcwidth.h
)

# Seems Qt Creator doesn't heed this?
#source_group(
#  TREE "${termic_SOURCE_DIR}/include"
#  PREFIX "Header Files"
#  FILES ${lib_headers}
#)

set(lib_sources
	app.cpp
	canvas.cpp
	look.cpp
	input.cpp
	keycodes.cpp
	samplers.cpp
	screen.cpp
	screen-buffer.cpp
	terminal.cpp
	utf8.cpp
	text.cpp
	../extern/mk-wcwidth/mk-wcwidth.cpp
)

add_library(termic STATIC ${lib_sources} ${lib_headers})

target_compile_options(termic
	PRIVATE
	-Wextra
	-Wall
	-Wpedantic
	-Werror
	--pedantic-errors
	-Wconversion
	-Wmissing-declarations
	-Wold-style-cast
	-Wno-padded
)

FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 8.1.1)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(signals GIT_REPOSITORY https://github.com/TheWisp/signals GIT_TAG 7ff59d1bc6387337ef34856a450be34cfa2c9e51)
FetchContent_MakeAvailable(signals)

# everyone need these include directories
#target_include_directories(termic PUBLIC ../extern/TheWisp-signals)
#target_include_directories(termic PUBLIC ../include)
target_include_directories(termic PUBLIC ../extern/mk-wcwidth)

target_include_directories(termic
	PUBLIC
	../include/
	${fmt_SOURCE_DIR}/include
	${signals_SOURCE_DIR}
)
