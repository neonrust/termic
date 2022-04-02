#pragma once

#include <cstdint>

#include "cell.h"
#include "screen-buffer.h"
#include "size.h"

namespace termic
{

enum Alignment
{
	Left = 0,
	Center,
	Right
};

//struct RegionI
//{
//  virtual Rectangle rect() const = 0;
//	virtual inline void clear() = 0;
//	virtual void clear(Color bg, Color fg=color::NoChange) = 0;
//	virtual void clear(const Rectangle &rect, Color bg, Color fg=color::NoChange) = 0;

//	virtual void go_to(Pos pos) = 0;
//	virtual std::size_t print(std::string_view s, Color fg=color::Default, Color bg=color::NoChange, Style style=style::Default) = 0;
//	virtual std::size_t print(Alignment align, Pos anchor_pos, std::string_view s, Color fg=color::Default, Color bg=color::NoChange, Style style=style::Default) = 0;
//	virtual std::size_t print(Pos pos, std::string_view s, Color fg=color::Default, Color bg=color::NoChange, Style style=style::Default) = 0;
//};

//struct Region;

struct Screen //: public RegionI
{
	Screen(int fd);

//	Region region(Rectangle rect) const; // TODO: what about resizing? need to be able to define position/size as fixed or percentage of parent
	void invalidate();

	inline void clear()  { clear(color::Default, color::Default); }
	void clear(Color bg, Color fg=color::NoChange);
	void clear(const Rectangle &rect, Color bg, Color fg=color::NoChange);

	void go_to(Pos pos);
	inline std::size_t print(std::string_view s, Look lk=look::Default)
	{
		return print(_client_cursor, s, lk);
	}
	std::size_t print(Alignment align, Pos anchor_pos, std::string_view s, Look lk=look::Default);
	std::size_t print(Pos pos, std::string_view s, Look lk=look::Default);
	std::size_t print(Pos pos, std::size_t wrap_width, std::string_view s, Look lk=look::Default);

	void update();

	void set_size(Size size);
	inline Size size() const { return _back_buffer.size(); }
	inline Rectangle rect() const { return { { 0, 0 }, size() }; }

	std::size_t measure(std::string_view s) const;

	Cell pick(Pos pos) const;

private:
	friend struct Canvas;  // direct access to internals
	friend struct App;     // for get_terminal_size()  :(

	Size get_terminal_size();
	Cell &cell(Pos pos);
	const Cell &cell(Pos pos) const;
	void set_cell(Pos pos, std::string_view ch, std::size_t width, Look lk=look::Default);
	Pos cursor_move(Pos pos);
	void cursor_style(Style style);
	void cursor_set_look(Look lk);

	void _out(const std::string_view text);
	void flush_buffer();


private:
	Pos _client_cursor { 0, 0 };

	ScreenBuffer _back_buffer;
	ScreenBuffer _front_buffer; // are multiple layers needed also here?
	bool _dirty { false };

	struct Cursor
	{
		Pos position { 0, 0 };
		Look look;
	} _cursor;

	std::string _output_buffer;
	const int _fd { 0 };
};

//struct Region : public RegionI
//{
//	void clear() override;
//	void clear(Color bg, Color fg) override;
//	void clear(const Rectangle &rect, Color bg, Color fg) override;
//	void go_to(Pos pos) override;
//	std::size_t print(std::string_view s, Color fg, Color bg, Style style) override;
//	std::size_t print(Alignment align, Pos anchor_pos, std::string_view s, Color fg, Color bg, Style style) override;
//	std::size_t print(Pos pos, std::string_view s, Color fg, Color bg, Style style) override;

//private:
//	Region(Screen &screen, Rectangle rect);

//private:
//	Screen &_screen;
//	Rectangle _rect;
//};


} // NS: termic
