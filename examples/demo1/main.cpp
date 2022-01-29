#include <termic/app.h>
#include <termic/canvas.h>
#include <termic/samplers.h>
#include <termic/utf8.h>
using namespace termic;

#include <tuple>
#include <fmt/core.h>
#include <fmt/format.h>
using namespace fmt::literals;
#include <variant>
#include <cmath>

#include <string>
using namespace std::literals;

#include <mk-wcwidth.h>

namespace termic
{
extern std::FILE *g_log;
}


int main()
{
//	const auto u8test { "利Ö治Aミ|"sv };
//	fmt::print("utf-8 input sequence: {} bytes\n", u8test.size());
//	auto cpiter = utf8::CodepointIterator(u8test);
//	for(auto iter = utf8::SequenceIterator(u8test); iter.good(); ++iter)
//	{
//		const auto &ch = *iter;
//		fmt::print("codepoint: \x1b[33;1m{}\x1b[m \\u{:04x} [", ch, *cpiter);
//		for(const auto &chc: ch)
//			fmt::print("{:02x} ", static_cast<std::uint8_t>(chc));
//		fmt::print("]\n");
//		++cpiter;
//	}
//	//fmt::print(u8"{}", std::u8string(u8"\xc3\x85"));

//	fmt::print("width: {}\n", ::mk_wcwidth(0xd6));


//	exit(1);



	g_log = fopen("termic.log", "w");
	::setbuf(g_log, nullptr);  // disable buffering

	App app(HideCursor | MouseEvents);// | FocusEvents);
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

	auto render_demo = [&app, &rotation, &gradient, &offset](key::Key key=key::None, key::Modifier mods=key::NoMod, const event::MouseButton *mb=nullptr) {

		auto &screen = app.screen();
		screen.clear();
		(void)key;
		(void)mods;
		(void)mb;

		Canvas canvas(screen);
		const auto &[width, height] = screen.size();

		screen.print({ 10, 14 }, "Termic rainbow demo!", color::White);
		gradient.set_offset(offset);
		canvas.fill(&gradient, rotation);
		screen.print({ width/2 - 3, 10 }, "TERMIC", color::Green);
		screen.print({ 50, 22 }, "Try arrow keys", color::Black);
		screen.print({ 40, 24 }, "Also try resizing the terminal", color::Black);

		screen.print({ 10, 19 }, "0123456789", color::Grey);
		const auto w = screen.print({ 10, 20 }, "利Ö治Aミ|", { color::White, style::Default, color::Black });
		screen.print({ 10, 21 }, "width of above: {}"_format(w), { color::Grey, style::Default, color::Black });

		if(key != key::None)
		{
			const auto key_str = key::to_string(key, mods);
			screen.print({ 25, 17 }, "Key pressed: {}"_format(key_str), color::White);
		}

		if(mb)
			screen.print({ 25, 18 },
						 "Mouse button {} {} @ {},{} mods: {}"_format(
							 mb->button,
							 mb->double_clicked? "double-clicked": (mb->pressed? "pressed": "released"),
							 mb->x,
							 mb->y,
							 key::to_string(key::None, mb->modifiers)
						 ),
						 color::White
						);


		screen.print(Left, { 0, 1 }, "This text is left-aligned", color::Black);
		screen.print(Center, { width/2, 2 }, "This text is center-aligned", color::Black);
		screen.print(Right, { width - 1, 3 }, "This text is right-aligned", color::Black);

		if(g_log) fmt::print(g_log, "render_demo\n");
	};

//	app.on_app_start.connect([&render_demo]() {
//		render_demo();
//	});

	app.on_app_exit.connect([](int rc) {
		fmt::print("termic::App exit ({})\n", rc);
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
	app.on_mouse_move_event.connect([&app](const event::MouseMove &mm) {
//		fmt::print(g_log, "[main]  mouse: {},{}\n", mm.x, mm.y);

		auto &screen = app.screen();
		Canvas canvas(screen);
		screen.clear();

		const auto mmx = mm.x;

		const auto &[width, height] = screen.size();

		static const color::LinearGradient shade { color::rgb(20, 20, 20), color::rgb(64, 64, 64) };

		const auto wrap_width = mmx + 1;
		Rectangle rect { { 0, 0 }, { wrap_width, height } };
		canvas.fill(rect, &shade);
//		screen.print({ 0, 0 }, wrap_width, "This text   is testing word-wrapping, yeah.", color::White);
		screen.print({ 0, 0 }, wrap_width, "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.", color::White);
//		screen.print({ 0, 0 }, wrap_width, "word-wrapping,", color::White);

		screen.print(Right, { width - 1, 0 }, "width: {}  ({}, {})"_format(wrap_width, mmx, mm.y));
	});
	app.on_mouse_button_event.connect([&render_demo](const event::MouseButton &mb) {
		fmt::print(g_log, "[main] button: {} {} @ {},{}\n",
				mb.button,
				mb.double_clicked? "double-click": (mb.pressed? "pressed": "released"),
				mb.x,
				mb.y
		);
		if(mb.pressed)
			render_demo(key::None, key::NoMod, &mb);
	});
	app.on_mouse_wheel_event.connect([](const event::MouseWheel &mw) {
		fmt::print(g_log, "[main]  wheel: {}\n", mw.delta);
	});
	app.on_resize_event.connect([&render_demo](const event::Resize &) {
		render_demo();
	});

	int rc { app.run() };

	fmt::print("demo1 main exit\n");

	return rc;
}


