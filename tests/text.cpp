#include <termic/text.h>
using namespace  termic;

using namespace std::literals;

#include <catch2/catch.hpp>

// TODO: use some testing lib, like gtest

TEST_CASE("Size of UTF-8 strings", "text::size") {
	REQUIRE(text::size("") == 0 );
	REQUIRE(text::size("hello") == 5 );
	REQUIRE(text::size("héllö") == 5 );
	REQUIRE(text::size("隊ぎやレね") == 5);
}

TEST_CASE("Insert into UTF-8 strings", "text::insert") {
	{
		utf8::string s { "" };
		REQUIRE(text::insert(s, "hello", 0) == "hello" );
	}
	{
		utf8::string s { "" };
		REQUIRE(text::insert(s, "hello", 42) == "hello" );
	}
	{
		utf8::string s { "llo" };
		REQUIRE(text::insert(s, "he", 0) == "hello" );
	}
	{
		utf8::string s { "heo" };
		REQUIRE(text::insert(s, "ll", 2) == "hello" );
	}
	{
		utf8::string s { "héo" };
		REQUIRE(text::insert(s, "ll", 2) == "héllo" );
	}
	{
		utf8::string s { "ho" };
		REQUIRE(text::insert(s, "隊ぎやレね", 1) == "h隊ぎやレねo");
	}
}

TEST_CASE("Erase from UTF-8 strings", "text::erase") {
	{
		utf8::string s { "" };
		REQUIRE(text::erase(s, 0, 10) == "" );
	}
	{
		utf8::string s { "hello" };
		REQUIRE(text::erase(s, 0, 2) == "llo" );
	}
	{
		utf8::string s { "hello" };
		REQUIRE(text::erase(s, 3, 2) == "hel" );
	}
	{
		utf8::string s { "héllo" };
		REQUIRE(text::erase(s, 2, 1) == "hélo" );
	}
	{
		utf8::string s { "隊ぎやレね" };
		REQUIRE(text::erase(s, 1, 2) == "隊レね");
	}
}
