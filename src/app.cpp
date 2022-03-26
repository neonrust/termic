#include <termic/app.h>
#include <termic/terminal.h>

#include <iostream>  // std::cin
#include <csignal>
#include <chrono>
#include <atomic>
using namespace std::literals;

#include <assert.h>

namespace termic
{
std::FILE *g_log;

void signal_received(int signum);
void app_atexit();

static App *g_app { nullptr };

App::App(Options opts) :
	timer(this),
	_input(std::cin),
	_screen(STDOUT_FILENO)
{
	assert(g_app == nullptr);
	g_app = this;

	term::init(STDIN_FILENO, STDOUT_FILENO, opts);
	_initialized = true;

	::atexit(app_atexit);
	std::signal(SIGINT, signal_received);
	std::signal(SIGTERM, signal_received);
	std::signal(SIGABRT, signal_received);
	std::signal(SIGFPE, signal_received);
	std::signal(SIGWINCH, signal_received);

//	_input.timer.connect([this]() {
//		on_timer();
//		_screen.update();
//	});
}

void app_atexit()
{
	if(g_app)
		g_app->shutdown();
}

App::~App()
{
	g_app = nullptr;
	this->shutdown();
}

App &App::the()
{
	assert(g_app != nullptr);
	return *g_app;
}

//App::operator bool() const
//{
//	return _initialized;
//}

using std::chrono::duration_cast;

int App::run()
{
	std::size_t prev_mx { static_cast<std::size_t>(-1) };
	std::size_t prev_my { static_cast<std::size_t>(-1) };

	_emit_resize_event = true;  // to emit the initial resize

	while(not _should_quit)
	{
		if(_emit_resize_event)
		{
			_emit_resize_event = false;

			const auto old_size = _screen.size();
			const auto first_resize = old_size.empty();

			const auto new_size = _screen.get_terminal_size();

			_screen.set_size(new_size);

			dispatch_event(event::Resize{
				.size = new_size,
				.old = {
					.size = old_size,
				},
			});

			// the first resize event also means that we've just started
			if(first_resize)
				on_app_start();

			if(first_resize)
				_screen.update();
		}

		for(const auto &event: _input.read())
		{
			const auto *mm = std::get_if<event::MouseMove>(&event);
			if(mm != nullptr)
			{
				if(mm->x == prev_mx and mm->y == prev_my)
					continue;

				prev_mx = mm->x;
				prev_my = mm->y;
			}

			dispatch_event(event);
		}

		_screen.update();
	}

	if(g_log) fmt::print(g_log, "\x1b[33;1mApp:loop exiting\x1b[m\n");

	return 0;
}

void App::trigger_render()
{
	_input.trigger_render();
}

void App::quit()
{
	_should_quit = true;
	_input.cancel_all_timers();
}

void App::shutdown(int rc)
{
	on_app_exit(rc);

	if(_initialized)
	{
		_initialized = false;
		term::restore(STDIN_FILENO, STDOUT_FILENO);
	}
}

bool App::dispatch_event(const event::Event &e)
{
	if(std::holds_alternative<event::MouseMove>(e))
		return on_mouse_move_event(std::get<event::MouseMove>(e)), true;
	else if(std::holds_alternative<event::MouseWheel>(e))
		return on_mouse_wheel_event(std::get<event::MouseWheel>(e)), true;
	else if(std::holds_alternative<event::Resize>(e))
		return on_resize_event(std::get<event::Resize>(e)), true;
	else if(std::holds_alternative<event::Key>(e))
		return on_key_event(std::get<event::Key>(e)), true;
	else if(std::holds_alternative<event::Input>(e))
		return on_input_event(std::get<event::Input>(e)), true;
	else if(std::holds_alternative<event::MouseButton>(e))
		return on_mouse_button_event(std::get<event::MouseButton>(e)), true;
	else if(std::holds_alternative<event::Focus>(e))
		return on_focus_event(std::get<event::Focus>(e)), true;
	else if(std::holds_alternative<event::Render>(e))
		return on_render(), true;

	if(g_log) fmt::print(g_log, "unhandled event type index:{}\n", e.index());

	return false;
}

void signal_received(int signum)
{
	if(signum == SIGWINCH)
	{
		if(g_app)
			g_app->_emit_resize_event = true;
		return;
	}

	if(g_log) fmt::print(g_log, "\x1b[33;1msignal: {}\x1b[m\n", signum);

	if(g_app)
		g_app->shutdown(128 + signum);
	g_app = nullptr;

	std::signal(signum, SIG_DFL);
	std::raise(signum);
}

// TimerAPI ---------------------------------------------------------

Timer App::TimerAPI::set(std::chrono::milliseconds initial, std::chrono::milliseconds interval, std::function<void ()> callback)
{
	return _app->_input.set_timer(initial, interval, callback);
}

void App::TimerAPI::cancel(const Timer &t)
{
	_app->_input.cancel_timer(t);
}

} // NS: termic
