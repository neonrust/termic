#pragma once

#include <iostream>
#include <optional>

#include "event.h"

namespace termic
{

struct Input
{
	Input(std::istream &s);

	std::vector<event::Event> read();

private:
	bool setup_keys();

private:
	std::istream &_in;

	struct KeySequence
	{
		std::string_view sequence;
		key::Modifier mods { key::NoMod };
		key::Key key;
	};
	std::vector<KeySequence> _key_sequences;
};

} // NS: termic
