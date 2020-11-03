#include "Narration.h"

#include "engine/Windows.h"
#include "engine/Controller.h"
#include "engine/Menu.h"

#include "Behaviours.h"

#include "engine/stdc++.h"

Narration::Narration(int x, int y, int resize, bool bMenu, std::string filename) {
	setName("narr");
	bPlay = false;

	bPrompt = false;
	prompt = nullptr;

	table = new GUI_WindowsTable("narrationui", getwidth()-35);
	
	win = new GUI_Window(30, filename, x, y, false, resize);
	win->setFunction("0", GUI_FUNCTION::STRDISPLAY, this, &Narration::textAnimationProvider);
	win->formatNewestComponent();
	win->setFunction("1", GUI_FUNCTION::STRDISPLAY, this, &Narration::getExplanationText);
	win->setNewestComponentColor(GRAY);
	win->setFunction("2", GUI_FUNCTION::OBJDISPLAY, this, &Narration::getAvatar);
	win->setName("narrWin");

	ctrl = new Controller(10);
	ctrl->setToggleEvent(KEY_RELEASE, "ENTER", this, &Narration::controllerEnterRelease);
	ctrl->setName("narrCtrl");

	avatar = nullptr;
	bSkip = false;
	menu = nullptr;

	if (bMenu) {
		menu = new GUI_Menu(std::vector <std::string>({ "W", "UP" }), std::vector <std::string>({ "S", "DOWN" }), 2, getheight() - 9);
		menu->bAutoDestroy = false;
		menu->setName("narrMenu");
		
		addBlankButton(2, 3, &Narration::getOption <1>, &Narration::getOption <1>, WHITE, YELLOW);
		addBlankButton(2, 4, &Narration::getOption <2>, &Narration::getOption <2>, WHITE, YELLOW);
		addBlankButton(2, 5, &Narration::getOption <3>, &Narration::getOption <3>, WHITE, YELLOW);
		addBlankButton(2, 6, &Narration::getOption <4>, &Narration::getOption <4>, WHITE, YELLOW);
		menu->freeze();
	}
}	

Narration::~Narration() {
	if (table) delete table;
}

void Narration::addBlankButton(int x, int y, std::string(Narration::* f1)(), std::string(Narration::* f2)(), COLOR unhovered, COLOR hovered) {
	GUI_Window* win = new GUI_Window(35, "", 0, 0);
	win->setName("narrBtn");

	table->updateWindow(win, 0);
	win->setFunction("0", STRDISPLAY, this, f1);
	win->color = unhovered;
	win->setNewestComponentColor(unhovered);
	win->hide();
	GUI_Window* hover_win = new GUI_Window(35, "", 0, 0);
	hover_win->setName("narrBtn");

	table->updateWindow(hover_win, 0);
	hover_win->setFunction("0", STRDISPLAY, this, f2);
	hover_win->color = hovered;
	hover_win->setNewestComponentColor(hovered);
	hover_win->hide();
	menu->addButton(win, hover_win, x, y, this, &Narration::menuButtonPress);
}

void Narration::loop(float delta) {
	PrimitiveObject::loop(delta);

	if (prompt) {
		ctrl->freeze();
		if (!prompt->isPromptAlive()) {
			if (prompt->answer == "x") {
				selection = 1;
				bPrompt = false;
				avatar = nullptr;
				bPlay = false;
			}
			else {
				selection = -1;
			}
			prompt->destroy();
			prompt = nullptr;
		}
	}
	else {
		if (bPlay) {
			ctrl->wakeUp();
			time += delta;
		}
		else {
			ctrl->freeze();
		}
	}
}


void Narration::playText(std::string text) {
	for (int i = 0; i < 4; ++i)
		options[i] = "";
	bSkip = true;
	noptions = 0;
	explanation = "  ENTER to SKIP/CONTINUE";
	win->setFboxColor(1, GRAY);
	time = 0;
	bPlay = true;
	selection = -1;
	prevchars = 0;
	idx = 0;
	if (menu) menu->hide();
	if (menu) menu->freeze();
	this->text = text;
	optionOffset = 0;
	optionMap.clear();
}
void Narration::addOption(std::string str) {
	++ optionOffset;
	if (str != "x") {
		menu->hover = 0;
		++noptions;
		options[noptions - 1] = "[ " + intToStr(noptions) + ". " + str + " ]";
		optionMap[noptions] = optionOffset;
	}
}

void Narration::setPrompt(std::string text) {
	bPrompt = true;
	promptText = text;
}

void Narration::destroy() {
	PrimitiveObject::destroy();
	if (menu) menu->destroy();
	if (ctrl) ctrl->destroy();
	if (win) win->destroy();
	if (prompt) prompt->destroy();
}

void Narration::controllerEnterRelease() {
	if (idx + 1 < (int)text.size()) {
		if (bSkip) idx = text.size() - 1;
	}
	else {
		if (noptions == 0) {
			bPlay = false;
			selection = 0;
			avatar = nullptr;
		}
	}
}

void Narration::disableSkip() {
	bSkip = false; 
	explanation = "       CANNOT SKIP"; 
	win->setFboxColor(1, YELLOW);
}

PromptAsker* Narration::createPrompt() { return new PromptAsker(new Prompt("prompt", getwidth() / 2 - 40, getheight() / 2 - 4 - 11, false, 80), 1); }

std::string Narration::textAnimationProvider() {
	int size = text.size();
	int chars = (int)(charPerSecond * time);
	while (prevchars < chars && idx < size-1) {
		if (pauseSigns.find(text[idx]) != pauseSigns.npos || (text[idx] == '-' && idx < size && text[idx + 1] == ' ')) {
			prevchars += delay;
			if (strongSigns.find(text[idx]) != pauseSigns.npos)
				prevchars += bonus;
		}
		else {
			prevchars += 1;
		}	++idx;
	}

	int len = (idx+1 < size ? idx+1 : size);

	std::string ans = text.substr(0, len);
	if (idx + 1 >= (int)text.size()) {
		if (bPrompt) {
			if (prompt == nullptr) {
				prompt = createPrompt();
			}
		}	else
		if (bPlay && noptions > 0) {
			menu->show();
			menu->wakeUp();
			
			explanation = "   SELECT your option";
			menu->restriction = noptions;
		}
		else {
			menu->freeze();
			explanation = "  ENTER to SKIP/CONTINUE";
		}
		win->setFboxColor(1, WHITE);
	}

	return ans;
}

void Narration::hideWindow() {
	win->hide();
}

void Narration::showWindow() {
	win->show();
}

void Narration::scrollOpen() {
	menu->hide();
	new ScrollAppearance <GUI_Window>(20.0f, win);
}

void Narration::menuButtonPress() {
	bPlay = false;
	for (int i = 0; i < 4; ++i)
		options[i] = "";
	selection = menu->hover+1;
	avatar = nullptr;
	menu->freeze();
}
