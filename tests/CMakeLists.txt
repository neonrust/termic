set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wextra -Wall -Wpedantic -Wconversion -Werror -Wno-padded)

set(sources
	text.cpp
)

add_executable(test_text ${sources})
target_link_libraries(test_text PRIVATE termic fmt pthread dl)

add_test(NAME text COMMAND test_text)
