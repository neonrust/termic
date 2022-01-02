#pragma once

#include <chrono>
#include <cstdint>

struct StopWatch
{
	inline StopWatch()
	{
		reset();
	}

	inline void reset()
	{
		_start_time = std::chrono::high_resolution_clock::now();
	}

	inline std::chrono::duration<double> elapsed() const
	{
		return std::chrono::high_resolution_clock::now() - _start_time;
	}

	inline std::uint64_t elapsed_us() const
	{
		return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::microseconds>(elapsed()).count());
	}

	inline double elapsed_s() const
	{
		return static_cast<double>(std::chrono::duration_cast<std::chrono::nanoseconds>(elapsed()).count())/1e9;
	}


private:
	std::chrono::time_point<std::chrono::high_resolution_clock>  _start_time;
};
