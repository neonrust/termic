#pragma once

#include <vector>
#include <memory>

namespace termic
{

struct Screen;
struct Panel;

enum Docking
{
	DockAll,
	DockLeft,
	DockRight,
	DockTop,
	DockBottom,
};


struct Layout
{
	Layout(Screen &scr);

	Panel *create_panel(Docking docking=DockAll);

private:
	Screen &_scr;

	std::vector<std::shared_ptr<Panel>> _panel;
};


} // NS: termic
