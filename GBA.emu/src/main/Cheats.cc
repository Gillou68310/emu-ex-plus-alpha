#include <imagine/gui/TextEntry.hh>
#include <emuframework/Cheats.hh>
#include <emuframework/MsgPopup.hh>
#include <emuframework/EmuApp.hh>
#include "EmuCheatViews.hh"
#include <gba/Cheats.h>
static bool cheatsModified = false;

void SystemEditCheatView::renamed(const char *str)
{
	cheatsModified = true;
	auto &cheat = cheatsList[idx];
	string_copy(cheat.desc, str);
	name.t.setString(cheat.desc);
	name.compile(projP);
}

void SystemEditCheatView::removed()
{
	cheatsModified = true;
	cheatsDelete(gGba.cpu, idx, true);
	refreshCheatViews();
}

void SystemEditCheatView::init(int cheatIdx)
{
	idx = cheatIdx;
	auto &cheat = cheatsList[idx];

	uint i = 0;
	loadNameItem(cheat.desc, item, i);

	name_ = "Edit Code";
	code.init(cheat.codestring); item[i++] = &code;

	loadRemoveItem(item, i);
	assert(i <= sizeofArray(item));
	TableView::init(item, i);
}

SystemEditCheatView::SystemEditCheatView(Base::Window &win): EditCheatView("", win),
	code
	{
		"Code",
		[this](DualTextMenuItem &item, View &, Input::Event e)
		{
			popup.post("To change this cheat, please delete and re-add it");
		}
	}
{}

void EditCheatListView::loadAddCheatItems(std::vector<MenuItem*> &item)
{
	addGS12CBCode.init(); item.emplace_back(&addGS12CBCode);
	addGS3Code.init(); item.emplace_back(&addGS3Code);
}

void EditCheatListView::loadCheatItems(std::vector<MenuItem*> &item)
{
	uint cheats = cheatsNumber;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back();
		cheat[c].init(cheatsList[c].desc); item.emplace_back(&cheat[c]);
		cheat[c].onSelect() =
			[this, c](TextMenuItem &, View &, Input::Event e)
			{
				auto &editCheatView = *new SystemEditCheatView{window()};
				editCheatView.init(c);
				viewStack.pushAndShow(editCheatView, e);
			};
	}
}

void EditCheatListView::addNewCheat(int isGSv3)
{
	if(cheatsNumber == EmuCheats::MAX)
	{
		popup.postError("Too many cheats, delete some first");
		window().postDraw();
		return;
	}
	auto &textInputView = *new CollectTextInputView{window()};
	textInputView.init(isGSv3 ? "Input xxxxxxxx yyyyyyyy" : "Input xxxxxxxx yyyyyyyy (GS) or xxxxxxxx yyyy (AR)", getCollectTextCloseAsset());
	textInputView.onText() =
		[this, isGSv3](CollectTextInputView &view, const char *str)
		{
			if(str)
			{
				char tempStr[20];
				string_copy(tempStr, str);
				string_toUpper(tempStr);
				if(strlen(tempStr) == 17 && tempStr[8] == ' ')
				{
					logMsg("removing middle space in text");
					memmove(&tempStr[8], &tempStr[9], 9); // 8 chars + null byte
				}
				if(isGSv3 ?
					cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", true) :
					((strlen(tempStr) == 16 && cheatsAddGSACode(gGba.cpu, tempStr, "Unnamed Cheat", false))
					|| cheatsAddCBACode(gGba.cpu, tempStr, "Unnamed Cheat")))
				{
					logMsg("added new cheat, %d total", cheatsNumber);
				}
				else
				{
					popup.postError("Invalid format");
					return 1;
				}
				cheatsModified = true;
				cheatsDisable(gGba.cpu, cheatsNumber-1);
				view.dismiss();
				auto &textInputView = *new CollectTextInputView{window()};
				textInputView.init("Input description", getCollectTextCloseAsset());
				textInputView.onText() =
					[](CollectTextInputView &view, const char *str)
					{
						if(str)
						{
							string_copy(cheatsList[cheatsNumber-1].desc, str);
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
	modalViewController.pushAndShow(textInputView, {});
}

EditCheatListView::EditCheatListView(Base::Window &win):
	BaseEditCheatListView(win),
	addGS12CBCode
	{
		"Add Game Shark v1-2/Code Breaker Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			addNewCheat(false);
		}
	},
	addGS3Code
	{
		"Add Game Shark v3 Code",
		[this](TextMenuItem &item, View &, Input::Event e)
		{
			addNewCheat(true);
		}
	}
{}

void CheatsView::loadCheatItems(std::vector<MenuItem*> &item)
{
	uint cheats = cheatsNumber;
	cheat.clear();
	cheat.reserve(cheats);
	iterateTimes(cheats, c)
	{
		cheat.emplace_back();
		auto &cheatEntry = cheatsList[c];
		cheat[c].init(cheatEntry.desc, cheatEntry.enabled); item.emplace_back(&cheat[c]);
		cheat[c].onSelect() =
			[this, c](BoolMenuItem &item, View &, Input::Event e)
			{
				cheatsModified = true;
				item.toggle(*this);
				if(item.on)
					cheatsEnable(c);
				else
					cheatsDisable(gGba.cpu, c);
			};
	}
}

void writeCheatFile()
{
	if(!cheatsModified)
		return;

	auto filename = FS::makePathStringPrintf("%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName().data());

	if(!cheatsNumber)
	{
		logMsg("deleting cheats file %s", filename.data());
		FS::remove(filename.data());
		cheatsModified = false;
		return;
	}
	cheatsSaveCheatList(filename.data());
	cheatsModified = false;
}

void readCheatFile()
{
	auto filename = FS::makePathStringPrintf("%s/%s.clt", EmuSystem::savePath(), EmuSystem::gameName().data());
	if(cheatsLoadCheatList(filename.data()))
	{
		logMsg("loaded cheat file: %s", filename.data());
	}
}
