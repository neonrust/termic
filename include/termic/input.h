#pragma once

#include <optional>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <functional>
using namespace std::literals;
using namespace std::chrono;

#include "event.h"
#include "stopwatch.h"
#include "timer.h"

#include <signals.hpp>

#include <poll.h>


namespace termic
{

struct Input
{
	friend struct App;

	Input(std::istream &s);

	void set_double_click_duration(milliseconds duration);

	std::vector<event::Event> read();

	static constexpr std::size_t max_timers { 16 };
	static constexpr milliseconds min_timer_duration { 10ms };

private:
	// called by Api
	Timer set_timer(milliseconds initial, milliseconds interval, std::function<void ()> callback);
	void cancel_timer(const Timer &t);

	void build_pollfds();
	void cancel_all_timers();

private:
	bool wait_input_and_timers();
	bool setup_keys();
	std::variant<event::Event, int> parse_mouse(std::string_view in, std::size_t &eaten);
	void _cancel_timer(std::uint64_t id);

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

	// Timer id -> event fd
	std::unordered_map<std::uint64_t, int> _timer_id_fd;
	// event fd -> TimerInfo
	struct TimerInfo
	{
		std::function<void()> callback;
		bool single_shot;
		std::uint64_t id;
		std::shared_ptr<Timer::Data> data;
	};
	std::unordered_map<int, TimerInfo> _timer_info;
	std::mutex _timers_lock;

	::pollfd _pollfds[max_timers];
};

} // NS: termic
