/*  This file is part of Imagine.

	Imagine is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	Imagine is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with Imagine.  If not, see <http://www.gnu.org/licenses/> */

#pragma once

#include <imagine/engine-globals.h>
#include <imagine/gui/View.hh>
#include <imagine/gui/MenuItem.hh>
#include <imagine/gui/TableView.hh>
#include <imagine/util/rectangle2.h>
#include <imagine/util/DelegateFunc.hh>

class AlertView : public View
{
public:
	AlertView(Base::Window &win, const char *label, MenuItem **menuItem);
	AlertView(Base::Window &win, const char *label, MenuItem **menuItem, uint menuItems);
	IG::WindowRect &viewRect() override { return rect; }
	void deinit() override;
	void place() override;
	void inputEvent(Input::Event e) override;
	void draw() override;
	void onAddedToController(Input::Event e) override;

protected:
	Gfx::GCRect labelFrame{};
	Gfx::Text text{};
	TableView menu;
	IG::WindowRect rect{};
};

class YesNoAlertView : public AlertView
{
public:
	using InputDelegate = DelegateFunc<void (Input::Event e)>;

	YesNoAlertView(Base::Window &win, const char *label, const char *choice1 = {}, const char *choice2 = {});
	void deinit() override;
	// Optional delegates
	InputDelegate &onYes() { return onYesD; }
	InputDelegate &onNo() { return onNoD; }

protected:
	InputDelegate onYesD{};
	InputDelegate onNoD{};
	MenuItem *menuItem[2]{};
	TextMenuItem yes, no;
};
