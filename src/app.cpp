#include <termic/app.h>
#include <termic/terminal.h>

#include <csignal>
#include <chrono>
using namespace std::literals::chrono_literals;

#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <assert.h>

namespace termic
{
std::FILE *g_log;

void signal_received(int signum);
void app_atexit();

static App *g_app { nullptr };

App::App(Options opts) :
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

	_input.timer.connect([this]() {
		on_timer();
		_screen.update();
	});
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

App::operator bool() const
{
	return _initialized;
}

using std::chrono::duration_cast;

void App::set_timer_interval(std::chrono::nanoseconds ns)
{
	if(ns.count() == 0)
	{
		clear_timer();
		return;
	}

	if(_timer_fd == 0)
		_timer_fd = ::timerfd_create(CLOCK_MONOTONIC, 0);

	const auto seconds = duration_cast<std::chrono::seconds>(ns);
	const auto nano_seconds = duration_cast<std::chrono::nanoseconds>(ns - seconds);

	const ::timespec ts {
		.tv_sec = seconds.count(),
		.tv_nsec = nano_seconds.count(),
	};

	const ::itimerspec timer_interval {
		.it_interval = ts,
		.it_value = ts,
	};

	_input.set_timer_fd(_timer_fd);

	[[maybe_unused]] int rc = ::timerfd_settime(_timer_fd, 0, &timer_interval, nullptr);
	assert(rc == 0);
}

void App::clear_timer()
{
	if(_timer_fd)
		::close(_timer_fd);

	_timer_fd = 0;

	_input.set_timer_fd(0);
}

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

			const auto before = _screen.size();
			const bool first_resize = before.width == 0 and before.height == 0;

			const auto size = _screen.get_terminal_size();

			enqueue_resize_event(size);

			_screen.set_size(size);

			if(first_resize)
				on_app_start();
		}

		for(const auto &event: _internal_events)
			dispatch_event(event);
		_internal_events.clear();

		_screen.update();

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
	}

	if(g_log) fmt::print(g_log, "\x1b[33;1mApp:loop exiting\x1b[m\n");

	return 0;
}

void App::quit()
{
	set_timer_interval(0ms);
	_should_quit = true;
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
	if(std::holds_alternative<event::Key>(e))
		return on_key_event(std::get<event::Key>(e)), true;
	else if(std::holds_alternative<event::Input>(e))
		return on_input_event(std::get<event::Input>(e)), true;
	else if(std::holds_alternative<event::MouseButton>(e))
		return on_mouse_button_event(std::get<event::MouseButton>(e)), true;
	else if(std::holds_alternative<event::MouseMove>(e))
		return on_mouse_move_event(std::get<event::MouseMove>(e)), true;
	else if(std::holds_alternative<event::MouseWheel>(e))
		return on_mouse_wheel_event(std::get<event::MouseWheel>(e)), true;
	else if(std::holds_alternative<event::Resize>(e))
		return on_resize_event(std::get<event::Resize>(e)), true;
	else if(std::holds_alternative<event::Focus>(e))
		return on_focus_event(std::get<event::Focus>(e)), true;

	if(g_log) fmt::print(g_log, "unhandled event type index:{}\n", e.index());

	return false;
}

void App::enqueue_resize_event(Size size)
{
	_internal_events.emplace_back<event::Resize>({
	    .size = size,
	    .old = {
	        .size = _screen.size(),
	    },
	});
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

} // NS: termic
