#pragma once

#include "terminal.h"
#include "event.h"
#include "cell.h"
#include "size.h"
#include "input.h"
#include "screen.h"

#include <signals.hpp>

#include <chrono>
#include <functional>
using namespace std::literals;

namespace termic
{

struct App
{
	friend void signal_received(int signum);
	friend void app_atexit();

	App(Options opts=Defaults);
	virtual ~App();

	static App &the();

	struct TimerAPI
	{
		// prevent copying
		TimerAPI() = delete;
		TimerAPI(const TimerAPI &) = delete;
		TimerAPI &operator = (const TimerAPI &) = delete;

		inline Timer create(std::chrono::milliseconds duration, std::function<void()> callback)
		{
			return create(duration, 0s, callback);
		}
		inline Timer after(std::chrono::milliseconds duration, std::function<void()> callback)
		{
			return create(duration, 0s, callback);
		}
		Timer create(std::chrono::milliseconds initial, std::chrono::milliseconds interval, std::function<void()> callback);
		inline Timer every(std::chrono::milliseconds interval, std::function<void()> callback)
		{
			return create(interval, interval, callback);
		}

		void cancel(const Timer &t);

	private:
		friend struct App; // to call below ctor
		inline TimerAPI(App *app) : _app(app) {}

	private:
		App *_app;
	} timer;

	// app.timer.after(1s, ...)
	fteng::signal<void()> on_render;

	fteng::signal<void(const event::Key)> on_key_event;
	fteng::signal<void(const event::Input)> on_input_event;
	fteng::signal<void(const event::MouseMove)> on_mouse_move_event;
	fteng::signal<void(const event::MouseButton)> on_mouse_button_event;
	fteng::signal<void(const event::MouseWheel)> on_mouse_wheel_event;

	fteng::signal<void()> on_app_start;
	fteng::signal<void(int)> on_app_exit;
	fteng::signal<void(const event::Resize)> on_resize_event;
	fteng::signal<void(const event::Focus)> on_focus_event;

	virtual int run();

	void trigger_render();
	void quit();

	Screen &screen() { return _screen; }


private:
	void shutdown(int rc=0);
	bool dispatch_event(const event::Event &e);


private:
	Input _input;
	Screen _screen;

	bool _emit_resize_event { false };

	bool _initialized { false };

	bool _should_quit { false };
};

} // NS: termic
