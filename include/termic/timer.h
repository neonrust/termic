#pragma once

#include <cstdint>
#include <chrono>
#include <memory>
using namespace std::chrono;


namespace termic
{


// create with app.timer.*
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
	~Timer();

	inline operator bool () const { return _id > 0; }

	void cancel();

	bool cancel_on_death { false };


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


} // NS: termic

