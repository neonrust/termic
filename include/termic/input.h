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

#include <signals.hpp>

#include <poll.h>


namespace termic
{

struct Timer
{
	friend struct Input; // to call ctor

	struct Data
	{
		const milliseconds initial;
		const milliseconds interval;
		const system_clock::time_point creation_time;

		std::size_t trigger_count { 0 };
		std::size_t triggers_missed { 0 };
		system_clock::time_point last_trigger_time;
		milliseconds lag { 0 };
	};

	inline Timer() : _id(0) {}  // invalid
	inline std::uint64_t id() const { return _id; }

	inline operator bool () const { return _id > 0; }

	void cancel();


	// these methods are only valid on valid Timer instances (i.e. not default ctor)
	// initial delay (might be zero)
	inline milliseconds initial() const { return _data->initial; };
	// interval between multiple triggers (zero = timer is single-shot)
	inline milliseconds interval() const { return _data->interval; };
	// returns whether the timer is single-shot, or not
	inline bool is_single_shot() const { return _data->interval.count() == 0; }
	// time point the timer was created
	inline system_clock::time_point creation_time() const { return _data->creation_time; };
	// how many times the timer was successfully triggered (i.e. its callback was called)
	inline std::size_t trigger_count() const { return _data->trigger_count; };
	// number of triggers missied (i.e. if the timer is lagging behind)
	inline std::size_t triggers_missed() const { return _data->triggers_missed; };
	// last time the timer was successfully triggered (i.e. its callback was called)
	inline system_clock::time_point last_trigger_time() const { return _data->last_trigger_time; };
	// how much time the last trigger lagged  behind (can only be positive, or zero)
	inline milliseconds lag() const { return _data->lag; };

private:
	inline Timer(std::uint64_t id, std::shared_ptr<Data> data) : _id(id), _data(data) {}

private:
	std::uint64_t _id;
	std::shared_ptr<Data> _data;
};


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
