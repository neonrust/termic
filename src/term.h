#pragma once

#include <string>
#include <functional>
#include <cstdint>
#include <cwchar>
#include <optional>

#include "event.h"


using namespace std::literals::string_view_literals;
using namespace std::literals::string_literals;


namespace term
{

enum Options
{
	Defaults          = 0,
	Fullscreen        = 1 << 0,
	HideCursor        = 1 << 1,
	MouseButtonEvents = 1 << 2,
	MouseMoveEvents   = 1 << 3,
	MouseEvents       = MouseButtonEvents | MouseMoveEvents,
	NoSignalDecode    = 1 << 4,
};
// bitwise OR of multiple 'Options' is still an 'Options'
inline Options operator | (Options a, Options b)
{
	return static_cast<Options>(static_cast<int>(a) | static_cast<int>(b));
}

struct KeySequence
{
	std::string sequence;
	key::Modifier mods;
	key::Key key;
};

constexpr std::size_t max_color_seq_len { 16 };  // e.g. "8;5;r;g;b"
constexpr std::size_t max_style_seq_len { 6 };   // e.g. "1;2;3"

struct Cell
{
	bool dirty { false };
	char fg[max_color_seq_len] { '\0' };    // an already "compiled" sequence, e.g. "8;5;r;g;b"   (the '3' prefix is implied)
	char bg[max_color_seq_len] { '\0' };    // an already "compiled" sequence, e.g. "1"        (the '4' prefix is implied)
	char style[max_style_seq_len] { '\0' }; // an already "compiled" sequence, e.g. "1"
	wchar_t ch { '\0' };                    // a single UTF-8 character
	bool is_virtual { false };              // true: this cell is displaying content from its left neighbor (i.e. a double-width character)
};

using Color = std::string;
using Style = std::string;

struct App
{
	friend void signal_received(int signum);

	App(Options opts=Defaults);
	~App();

	operator bool() const;

	void loop(std::function<bool (const event::Event &)> handler);

	//std::shared_ptr<Surface> screen_surface();
	//std::shared_ptr<Surface> create_surface(std::size_t x, std::size_t y, std::size_t width, std::size_t height);

	void debug_print(std::size_t x, std::size_t y, Color fg, Color bg, Style st, const std::string &s);

	void clear();

private:
	bool initialize(Options opts);
	void shutdown();
	std::tuple<std::size_t, std::size_t> get_size() const;

	bool init_input();
	std::optional<event::Event> read_input() const;
	void shutdown_input();

	void enqueue_resize_event(std::tuple<std::size_t, std::size_t> size);
	void apply_resize(std::size_t width, std::size_t height);
	void refresh();
	void draw_cell(std::size_t x, std::size_t y, const Cell &cell, bool move_needed=true);
	void flush_buffer();

	void write(const std::string_view &s);

private:
	std::size_t _refresh_needed { 0 };
	std::vector<std::vector<Cell>> _cells;
	std::size_t _width { 0 };
	std::size_t _height { 0 };

	std::vector<KeySequence> _key_sequences;

	bool _resize_recevied { false };
	std::vector<event::Event> _internal_events;

	std::string _output_buffer;

	bool _fullscreen { false };
	bool _initialized { false };

};

} // NS: term

namespace esc
{

const auto esc { "\x1b"s };
const auto csi { esc + "[" };

const auto cuu { csi + "{:d}A" };
const auto cud { csi + "{:d}B" };
const auto cuf { csi + "{:d}C" };
const auto cub { csi + "{:d}D" };
const auto cup { csi + "{:d};{:d}H" };
const auto ed  { csi + "{}J" };

} // NS: esc
