set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wextra -Wall -Wpedantic -Wconversion -Werror -Wno-padded)

set(lib_headers
	../include/termic/app.h
	../include/termic/canvas.h
	../include/termic/cell.h
	../include/termic/color.h
	../include/termic/event.h
	../include/termic/input.h
	../include/termic/keycodes.h
	../include/termic/layout.h
	../include/termic/panel.h
	../include/termic/samplers.h
	../include/termic/screen.h
	../include/termic/screen-buffer.h
	../include/termic/size.h
	../include/termic/style.h
	../include/termic/terminal.h
)

# Seems Qt Creator doesn't heed this?
source_group(
  TREE "${termic_SOURCE_DIR}/include"
  PREFIX "Header Files"
  FILES ${lib_headers}
)

set(lib_sources
	app.cpp
	terminal.cpp
	samplers.cpp
	screen.cpp
	screen-buffer.cpp
	input.cpp
	canvas.cpp
	keycodes.cpp
)

add_library(termic STATIC ${lib_sources} ${lib_headers})

# everyone need these include directories
target_include_directories(termic PUBLIC ../extern/TheWhisp-signals)
target_include_directories(termic PUBLIC ../include)
