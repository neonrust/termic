#include <fmt/core.h>

#include <assert.h>

#include <termic/text.h>
using namespace  termic;


// TODO: use some testing lib, like gtest

int main(int argc, char *argv[])
{
	std::vector<std::string_view> args { argv + 1, argv + argc };

	fmt::print("size: empty\n");
	{
		utf8::string s("");
		assert(text::size(s) == 0);
	}

	fmt::print("size: ascii\n");
	{
		utf8::string s("hello");
		assert(text::size(s) == 5);
	}

	fmt::print("size: accent\n");
	{
		utf8::string s("héllö");
		assert(text::size(s) == 5);
	}

	fmt::print("size: japanese\n");
	{
		utf8::string s("利治ミ");
		assert(text::size(s) == 3);
	}

	return 0;
}
