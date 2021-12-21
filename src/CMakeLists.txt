set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wextra -Wall -Wpedantic -Wconversion -Werror -Wno-padded)

set(lib_headers
	../include/app.h
	../include/event.h
	../include/terminal.h
	../include/cell.h
	../include/size.h
	../include/samplers.h
	../include/screen.h
	../include/screen-buffer.h
	../include/input.h
	../include/canvas.h
	../include/keycodes.h
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
