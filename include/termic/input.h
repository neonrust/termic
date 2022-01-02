#pragma once

#include <iostream>
#include <optional>

#include "event.h"
#include "stopwatch.h"

namespace termic
{

struct Input
{
	Input(std::istream &s);

	void set_double_click_duration(float duration);

	std::vector<event::Event> read();

private:
	bool setup_keys();
	std::variant<event::Event, int> parse_mouse(const std::string_view in, std::size_t &eaten);

private:
	std::istream &_in;

	struct KeySequence
	{
		std::string_view sequence;
		key::Modifier mods { key::NoMod };
		key::Key key;
	};
	std::vector<KeySequence> _key_sequences;
	StopWatch _mouse_button_press;
	float _double_click_duration { 0.3f };
};

} // NS: termic
