#pragma once

#include <optional>
#include <chrono>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <functional>
using namespace std::literals;

#include "event.h"
#include "stopwatch.h"

#include <signals.hpp>

#include <poll.h>


namespace termic
{

struct Timer
{
	static constexpr std::uint64_t Invalid { 0 };

	inline Timer() : _id(Invalid) {}
	inline Timer(std::uint64_t id) : _id(id) {}

	inline std::uint64_t id() const { return _id; }

	inline bool valid() const { return _id != Invalid; }
	inline operator bool () const { return valid(); }

	void cancel();

private:
	std::uint64_t _id;
};


struct Input
{
	friend struct App;

	Input(std::istream &s);

	void set_double_click_duration(std::chrono::milliseconds duration);

	std::vector<event::Event> read();

	static constexpr std::size_t max_timers { 16 };
	static constexpr std::chrono::milliseconds min_timer_duration { 10ms };

private:
	// called by Api
	Timer set_timer(std::chrono::milliseconds initial, std::chrono::milliseconds interval, std::function<void ()> callback);
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
	};
	std::unordered_map<int, TimerInfo> _timer_info;
	std::mutex _timers_lock;

	::pollfd _pollfds[max_timers];
};

} // NS: termic
