set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wextra -Wall -Wpedantic -Wconversion -Werror -Wno-padded)

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

# everyone need these include directories
target_include_directories(termic PUBLIC ../extern/TheWisp-signals)
target_include_directories(termic PUBLIC ../include)
target_include_directories(termic PUBLIC ../extern/mk-wcwidth)
