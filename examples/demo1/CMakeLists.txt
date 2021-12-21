set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wextra -Wall -Wpedantic -Wconversion -Werror -Wno-padded)

set(sources
	main.cpp
)

add_executable(demo1 ${sources})
target_link_libraries(demo1 PRIVATE termic fmt pthread dl)
