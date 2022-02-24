set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wextra -Wall -Wpedantic -Wconversion -Werror -Wno-padded)

set(sources
	main.cpp
)


#FetchContent_Declare(fmt GIT_REPOSITORY https://github.com/fmtlib/fmt.git GIT_TAG 8.1.1)
#FetchContent_MakeAvailable(fmt)


add_executable(demo1 ${sources})
target_link_libraries(demo1 PRIVATE termic fmt::fmt pthread dl)
