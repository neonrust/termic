#include <termic/app.h>
#include <termic/canvas.h>
#include <termic/samplers.h>

#include <tuple>
#include <fmt/core.h>
#include <variant>
#include <cmath>

namespace termic
{
extern std::FILE *g_log;
}

int main()
{
	using namespace termic;

	g_log = fopen("termic.log", "w");
	::setbuf(termic::g_log, nullptr);  // disable buffering

	fmt::print(g_log, "term test app!\n");


	App app(Fullscreen | HideCursor | MouseEvents | FocusEvents);
	if(not app)
		return 1;

	Canvas canvas(app.screen());
	color::LinearGradient gradient({
		color::Black,
		color::rgb(180, 180, 20),
		color::rgb(20, 20, 180),
		color::rgb(180, 20, 20),
		color::rgb(20, 180, 180),
		color::rgb(180, 20, 180),
		color::rgb(180, 180, 180),
		color::rgb(20, 180, 180),
		color::Black,
	});

	float rotation { 45 };
	float offset { 0 };

	auto render_demo = [&app, &canvas, &rotation, &gradient, &offset](key::Key key=key::None, key::Modifier mods=key::NoMod) {
		canvas.clear();
		app.screen().print({ 10, 10 }, "Termic rainbow demo!", color::White, color::NoChange);
		gradient.set_offset(offset);
		canvas.fill(&gradient, rotation);
		app.screen().print({ 6, 12 }, "Things and stuff...", color::rgb(255, 50, 50), color::NoChange);
		app.screen().print({ 70, 20 }, "TERMIC", color::Green, color::NoChange);

		if(key != key::None)
		{
			const auto key_str = key::to_string(key, mods);
			app.screen().print({ 25, 15 }, fmt::format("Key pressed: {}", key_str), color::White, color::NoChange);
		}
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
		}
		else if(k.key == key::LEFT and k.modifiers == key::NoMod)
		{
			rotation = std::fmod(std::fmod(rotation - 2.f, 360.f) + 360.f, 360.f);
			fmt::print(g_log, "rotation: {}\n", rotation);
		}
		else if(k.key == key::UP and k.modifiers == key::NoMod)
			offset += 0.02f;
		else if(k.key == key::DOWN and k.modifiers == key::NoMod)
			offset -= 0.02f;

		render_demo(k.key, k.modifiers);
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


