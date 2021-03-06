#include <imagine/gui/TextEntry.hh>
#include <emuframework/Cheats.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <fceu/driver.h>
extern uint fceuCheats;
void EncodeGG(char *str, int a, int v, int c);
static const int UNCHANGED_VAL = -2;

void SystemEditCheatView::syncCheat(const char *newName)
{
	if(type)
	{
		int a, v, c;
		if(!FCEUI_DecodeGG(ggCodeStr, &a, &v, &c))
		{
			logWarn("error decoding GG code %s", ggCodeStr);
			a = 0; v = 0; c = -1;
		}
		if(!FCEUI_SetCheat(idx, newName, a, v, c, -1, 1))
		{
			logWarn("error setting cheat %d", idx);
		}
	}
	else
	{
		logMsg("setting comp %d", strlen(compStr) ? (int)strtoul(compStr, nullptr, 16) : -1);
		if(!FCEUI_SetCheat(idx,
				newName, strtoul(addrStr, nullptr, 16), strtoul(valueStr, nullptr, 16),
				strlen(compStr) ? strtoul(compStr, nullptr, 16) : -1, -1, 0))
		{
			logWarn("error setting cheat %d", idx);
		}
	}
}

void SystemEditCheatView::renamed(const char *str)
{
	syncCheat(str);
	FCEUI_GetCheat(idx, &nameStr, nullptr, nullptr, nullptr, nullptr, nullptr);
	name.t.setString(nameStr);
	name.compile(projP);
	refreshCheatViews();
}

void SystemEditCheatView::removed()
{
	assert(fceuCheats != 0);
	FCEUI_DelCheat(idx);
	fceuCheats--;
	refreshCheatViews();
}

void SystemEditCheatView::init(int cheatIdx)
{
	idx = cheatIdx;
	uint32 a;
	uint8 v;
	int compare;
	int gotCheat = FCEUI_GetCheat(cheatIdx, &nameStr, &a, &v, &compare, 0, &type);
	logMsg("got cheat with addr 0x%.4x val 0x%.2x comp %d", a, v, compare);

	uint i = 0;
	loadNameItem(nameStr, item, i);
	if(type)
	{
		name_ = "Edit Code";
		if(a == 0 && v == 0 && compare == -1)
			ggCodeStr[0] = 0;
		else
			EncodeGG(ggCodeStr, a, v, compare);
		ggCode.init(ggCodeStr); item[i++] = &ggCode;
	}
	else
	{
		name_ = "Edit RAM Patch";
		snprintf(addrStr, sizeof(addrStr), "%x", a);
		addr.init(addrStr); item[i++] = &addr;
		snprintf(valueStr, sizeof(valueStr), "%x", v);
		value.init(valueStr); item[i++] = &value;
		if(compare == -1)
			compStr[0] = 0;
		else
			snprintf(compStr, sizeof(compStr), "%x", compare);
		comp.init(compStr); item[i++] = &comp;
	}
	loadRemoveItem(item, i);
	assert(i <= sizeofArray(item));
	TableView::init(item, i);
}

static bool isValidGGCodeLen(const char *str)
{
	return strlen(str) == 6 || strlen(str) == 8;
}

SystemEditCheatView::SystemEditCheatView(Base::Window &win): EditCheatView("", win),
	addr
	{
		"Address",
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input 4-digit hex", addrStr, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						uint a = strtoul(str, nullptr, 16);
						if(a > 0xFFFF)
						{
							logMsg("addr 0x%X too large", a);
							popup.postError("Invalid input");
							window().postDraw();
							return 1;
						}
						string_copy(addrStr, a ? str : "0");
						syncCheat();
						addr.compile(projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	value
	{
		"Value",
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input 2-digit hex", valueStr, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						uint a = strtoul(str, nullptr, 16);
						if(a > 0xFF)
						{
							logMsg("val 0x%X too large", a);
							popup.postError("Invalid input");
							window().postDraw();
							return 1;
						}
						string_copy(valueStr, a ? str : "0");
						syncCheat();
						value.compile(projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	comp
	{
		"Compare",
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input 2-digit hex or blank", compStr, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(strlen(str))
						{
							uint a = strtoul(str, nullptr, 16);
							if(a > 0xFF)
							{
								logMsg("val 0x%X too large", a);
								popup.postError("Invalid input");
								window().postDraw();
								return 1;
							}
							string_copy(compStr, str);
						}
						else
						{
							compStr[0] = 0;
						}
						syncCheat();
						comp.compile(projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	ggCode
	{
		"GG Code",
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input Game Genie code", ggCodeStr, getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!isValidGGCodeLen(str))
						{
							popup.postError("Invalid, must be 6 or 8 digits");
							window().postDraw();
							return 1;
						}
						string_copy(ggCodeStr, str);
						syncCheat();
						ggCode.compile(projP);
						window().postDraw();
					}
					view.dismiss();
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	}
{}

void EditCheatListView::loadAddCheatItems(std::vector<MenuItem*> &item)
{
	addGG.init(); item.emplace_back(&addGG);
	addRAM.init(); item.emplace_back(&addRAM);
}

void EditCheatListView::loadCheatItems(std::vector<MenuItem*> &item)
{
	uint cheats = fceuCheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back();
		char *name;
		int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, 0, 0);
		assert(gotCheat);
		if(!gotCheat) continue;
		cheat[c].init(name); item.emplace_back(&cheat[c]);
		cheat[c].onSelect() =
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				auto &editCheatView = *new SystemEditCheatView{window()};
				editCheatView.init(c);
				viewStack.pushAndShow(editCheatView, e);
			};
	}
}

EditCheatListView::EditCheatListView(Base::Window &win): BaseEditCheatListView(win),
	addGG
	{
		"Add Game Genie Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input Game Genie code", getCollectTextCloseAsset());
			textInputView.onText() =
				[this](CollectTextInputView &view, const char *str)
				{
					if(str)
					{
						if(!isValidGGCodeLen(str))
						{
							popup.postError("Invalid, must be 6 or 8 digits");
							return 1;
						}
						{
							int a, v, c;
							if(!FCEUI_DecodeGG(str, &a, &v, &c))
							{
								popup.postError("Invalid code");
								return 1;
							}
							if(!FCEUI_AddCheat("Unnamed Cheat", a, v, c, 1))
							{
								popup.postError("Error adding cheat");
								view.dismiss();
								return 0;
							}
						}
						fceuCheats++;
						FCEUI_ToggleCheat(fceuCheats-1);
						logMsg("added new cheat, %d total", fceuCheats);
						view.dismiss();
						auto &textInputView = *new CollectTextInputView{window()};
						textInputView.init("Input description", getCollectTextCloseAsset());
						textInputView.onText() =
							[](CollectTextInputView &view, const char *str)
							{
								if(str)
								{
									FCEUI_SetCheat(fceuCheats-1, str, UNCHANGED_VAL, UNCHANGED_VAL, UNCHANGED_VAL, -1, 1);
									view.dismiss();
									refreshCheatViews();
								}
								else
								{
									view.dismiss();
								}
								return 0;
							};
						refreshCheatViews();
						modalViewController.pushAndShow(textInputView, {});
					}
					else
					{
						view.dismiss();
					}
					return 0;
				};
			modalViewController.pushAndShow(textInputView, e);
		}
	},
	addRAM
	{
		"Add RAM Patch",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			auto &textInputView = *new CollectTextInputView{window()};
			textInputView.init("Input description", getCollectTextCloseAsset());
			textInputView.onText() =
			[](CollectTextInputView &view, const char *str)
			{
				if(str)
				{
					if(!FCEUI_AddCheat(str, 0, 0, -1, 0))
					{
						logErr("error adding new cheat");
						view.dismiss();
						return 0;
					}
					fceuCheats++;
					FCEUI_ToggleCheat(fceuCheats-1);
					logMsg("added new cheat, %d total", fceuCheats);
					auto &editCheatView = *new SystemEditCheatView{view.window()};
					view.dismiss();
					refreshCheatViews();
					// go to directly to cheat's menu to finish entering values
					editCheatView.init(fceuCheats-1);
					viewStack.pushAndShow(editCheatView, {});
				}
				else
				{
					view.dismiss();
				}
				return 0;
			};
			modalViewController.pushAndShow(textInputView, e);
		}
	}
{}

void CheatsView::loadCheatItems(std::vector<MenuItem*> &item)
{
	uint cheats = fceuCheats;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back();
		char *name;
		int status;
		int gotCheat = FCEUI_GetCheat(c, &name, 0, 0, 0, &status, 0);
		assert(gotCheat);
		if(!gotCheat) continue;
		cheat[c].init(name, status); item.emplace_back(&cheat[c]);
		cheat[c].onSelect() =
			[this, c](BoolMenuItem &item, View &, Input::Event e)
			{
				uint32 a;
				uint8 v;
				int compare, type;
				FCEUI_GetCheat(c, nullptr, &a, &v, &compare, 0, &type);
				if(!item.on && type && a == 0 && v == 0 && compare == -1)
				{
					// Don't turn on null Game Genie codes
					popup.postError("Game Genie code isn't set", 2);
					return;
				}
				item.toggle(*this);
				FCEUI_ToggleCheat(c);
			};
	}
}
