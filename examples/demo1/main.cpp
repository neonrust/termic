#include <termic/app.h>
#include <termic/canvas.h>
#include <termic/samplers.h>

#include <tuple>
#include <fmt/core.h>
#include <variant>
#include <cmath>

std::FILE *g_log { nullptr };

int main()
{
	g_log = fopen("termic.log", "w");
	::setbuf(g_log, nullptr);  // disable buffering

	fmt::print(g_log, "term test app!\n");

	using namespace termic;

	App app(Fullscreen | HideCursor | MouseEvents | FocusEvents);
	if(not app)
		return 1;

	Canvas canvas(app.screen());
	color::LinearGradient gradient({
		color::Black,
		color::Yellow,
		color::Blue,
		color::Red,
		color::Cyan,
		color::Purple,
		color::White,
		color::Green,
	});

	float rotation { 45 };
	float offset { 0 };

	auto render_demo = [&app, &canvas, &rotation, &gradient, &offset]() {
		canvas.clear();
		app.screen().print({ 10, 10 }, "Yeah!", color::Black, color::Unchanged);
		gradient.set_offset(offset);
		canvas.fill(&gradient, rotation);
		app.screen().print({ 6, 12 }, "Things and stuff...", color::Red, color::Unchanged);
		app.screen().print({ 70, 20 }, "TERMIC", color::Green, color::Unchanged);
	};

	app.on_app_start.connect([&render_demo]() {
		render_demo();
	});

	app.on_key_event.connect([&app, &render_demo, &rotation, &offset](const event::Key &k) {
		fmt::print(g_log, "[main]    key: {}\n", key::to_string(k.key, k.modifiers));

		if(k.key == key::ESCAPE and k.modifiers == key::NoMod)
			app.quit();

		if(k.key == key::RIGHT and k.modifiers == key::NoMod)
		{
			rotation = std::fmod(std::fmod(rotation + 2.f, 360.f) + 360.f, 360.f);
			fmt::print(g_log, "rotation: {}\n", rotation);
			render_demo();
		}
		else if(k.key == key::LEFT and k.modifiers == key::NoMod)
		{
			rotation = std::fmod(std::fmod(rotation - 2.f, 360.f) + 360.f, 360.f);
			fmt::print(g_log, "rotation: {}\n", rotation);
			render_demo();
		}
		else if(k.key == key::UP and k.modifiers == key::NoMod)
		{
			offset += 0.02f;
			render_demo();
		}
		else if(k.key == key::DOWN and k.modifiers == key::NoMod)
		{
			offset -= 0.02f;
			render_demo();
		}
	});
	app.on_input_event.connect([](const event::Input &c) {
		fmt::print(g_log, "[main]  input: '{}' 0x{:08x}\n", c.to_string(), std::uint32_t(c.codepoint));
		return true;
	});
	app.on_mouse_move_event.connect([](const event::MouseMove &mm) {
		fmt::print(g_log, "[main]  mouse: {},{}\n", mm.x, mm.y);
	});
	app.on_mouse_button_event.connect([](const event::MouseButton &mb) {
		fmt::print(g_log, "[main] button: {} {} @ {},{}\n",
				mb.button,
				mb.pressed? "pressed": "released",
				mb.x,
				mb.y
		);
	});
	app.on_mouse_wheel_event.connect([](const event::MouseWheel &mw) {
		fmt::print(g_log, "[main]  wheel: {}\n", mw.delta);
	});
	app.on_resize_event.connect([&render_demo](const event::Resize &) {
		render_demo();
	});

	return app.run();
}


