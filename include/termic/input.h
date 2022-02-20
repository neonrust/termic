#pragma once

#include <iostream>
#include <optional>

#include "event.h"
#include "stopwatch.h"

#include <signals.hpp>


namespace termic
{

struct Input
{
	Input(std::istream &s);

	void set_double_click_duration(float duration);

	std::vector<event::Event> read();

	inline void set_timer_fd(int fd) { _timer_fd = fd; }

	fteng::signal<void()> timer;

private:
	bool setup_keys();
	std::variant<event::Event, int> parse_mouse(std::string_view in, std::size_t &eaten);

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
	int _timer_fd { 0 };
};

} // NS: termic
