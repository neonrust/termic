#include <termic/text.h>
using namespace  termic;

#include <catch2/catch.hpp>

// TODO: use some testing lib, like gtest

TEST_CASE("Size of UTF-8 strings", "text::size") {
	REQUIRE( text::size("") == 0 );
	REQUIRE( text::size("hello") == 5 );
	REQUIRE( text::size("héllö") == 5 );
	REQUIRE( text::size("隊ぎやレね") == 5);
}
