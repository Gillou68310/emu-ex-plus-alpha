#pragma once
#include <emuframework/Cheats.hh>
#include <cheats.h>

namespace EmuCheats
{

static const uint MAX = MAX_CHEATS;

}

class SystemEditCheatView : public EditCheatView
{
private:
	DualTextMenuItem addr{}, value{}, saved{};
	uint idx = 0;
	MenuItem *item[5]{};
	char addrStr[7]{}, valueStr[3]{}, savedStr[3]{};

	void renamed(const char *str) override;
	void removed() override;

public:
	SystemEditCheatView(Base::Window &win);
	void init(int cheatIdx);
};

class EditCheatListView : public BaseEditCheatListView
{
private:
	TextMenuItem addCode{};
	std::vector<TextMenuItem> cheat{};

	void loadAddCheatItems(std::vector<MenuItem*> &item) override;
	void loadCheatItems(std::vector<MenuItem*> &item) override;

public:
	EditCheatListView(Base::Window &win);
};

class CheatsView : public BaseCheatsView
{
private:
	std::vector<BoolMenuItem> cheat{};

	void loadCheatItems(std::vector<MenuItem*> &item) override;

public:
	CheatsView(Base::Window &win): BaseCheatsView(win) {}
};
