#include "MainMenu.h"

#include "engine/Menu.h"
#include "engine/GodClass.h"
#include "engine/Timer.h"
#include "engine/Windows.h"
#include "engine/ConsoleRenderer.h"
#include "engine/ScreenObjectTable.h"
#include "engine/Debugger.h"

#include "Behaviours.h"
#include "Player.h"
#include "Characters.h"
#include "Scripts.h"

std::string display1() {
	return "PLAY DEMO";
}
std::string display1Hover() {
	return "P L A Y   D E M O";
}
std::string display2() {
	return "TEST RENDERER";
}
std::string display2Hover() {
	return "T E S T   R E N D E R E R";
}

MainMenu::MainMenu() {
	bScroll = true;
	timer = nullptr;

	menu = new GUI_Menu(std::vector <std::string> ({ "W", "UP" }), std::vector <std::string> ({ "S", "DOWN" }), 0, getheight() - 15, true);
	menu->bAutoDestroy = false;

	sh1 = new GUI_Window(50, "menuButton", 0, 0, false, 35);
	sh1->setFunction("0", STRDISPLAY, &display1, WHITE);
	
	sh1Hover = new GUI_Window(50, "menuButton", 0, 0, false, 35);
	sh1Hover->setFunction("0", STRDISPLAY, &display1Hover, YELLOW);
	
	sh2 = new GUI_Window(50, "menuButton", 0, 0, false, 35);
	sh2->setFunction("0", STRDISPLAY, &display2, WHITE);
	
	sh2Hover = new GUI_Window(50, "menuButton", 0, 0, false, 35);
	sh2Hover->setFunction("0", STRDISPLAY, &display2Hover, GREEN);

	menu->addButton(sh1, sh1Hover, getwidth()-50, 0, this, &MainMenu::pressDemoButton);
	menu->addButton(sh2, sh2Hover, getwidth()-50, 5, this, &MainMenu::pressRendererButton);

	ScreenObjectTable *scbt = new ScreenObjectTable("mainMenu");
	title = new ScreenObject(getwidth() - 65, getheight() - 21, 50, "");
	scbt->updateShape(title, "menuTitle");
	delete scbt;

	titlesCounter = new ScreenObject(5, getheight() - 9, 50, "");

	playAnimation();
}

void MainMenu::destroy() {	
	if (timer)		timer->destroy();
	if (menu)		menu->destroy();
	if (behaviour)	behaviour->destroy();
}

void MainMenu::wakeUp() {
	playAnimation();
	for (auto it : endingTitles)
		it->show();
	if (endingTitles.size() > 0) {
		titlesCounter->show();
		titlesCounter->updateShape(intToStr(endingTitles.size()) + "/3");
	}
	menu->show();
	menu->wakeUp();
	startingMotionFinished = false;
	bScroll = true;
}

void MainMenu::hide() {
	GOD->getRenderer()->resetBackground();
	PLAYERCH->getCharacter()->hide();
}

MainMenu::~MainMenu() { }

void MainMenu::loop(float delta) {
	PrimitiveObject::loop(delta);

	if (script && script->isOver()) {
		addEnding(script->getEnding());
		new Timer(0.6f, script, &DemoScript::destroy);
		script->hideMap();
		script = nullptr;
		fade();
		new Timer(1.0f, this, &MainMenu::wakeUp);
	}

	if (notifiers.find("movement behaviour finished") != notifiers.end()) {
		notifiers.erase("movement behaviour finished");
		GOD->getRenderer()->addScrollAmount(1.0f);
		startingMotionFinished = true;
	}

	if (startingMotionFinished && bScroll) GOD->getRenderer()->addScrollAmount(delta);
}

void MainMenu::addEnding(std::string ending) {
	if (endingsSet.find(ending) != endingsSet.end()) return;
	endingsSet.insert(ending);
	int size = endingTitles.size();
	endingTitles.push_back(new ScreenObject(5, getheight() - 8 + size, 50, ""));
	endingTitles.back()->updateShape(ending);
	endingTitles.back()->hide();
}

void MainMenu::playAnimation() {
	notifiers.clear();
	for (auto it : PLAYERCH->getParty()) it->hide();
	
	title->show();
	menu->show();

	GOD->getRenderer()->resetBackground();
	GOD->getRenderer()->addBackground("forestbg2", true, 0.5f);
	GOD->getRenderer()->addBackground("forestbg1", true, 1.3f);
	
	PLAYERCH->getCharacter()->setPosition(0, 30);
	std::vector <std::pair <int, int>> arr = { {0, 30}, { 40, 30 } };
	behaviour = new CombatGridLineTraceBehaviour <Character>(arr, PLAYERCH->getCharacter(), 8);
	
	PLAYERCH->getCharacter()->show();
	PLAYERCH->getCharacter()->requestAnimation("walk_guitar", "walk_guitar");

	new ScrollAppearance <GUI_Window>(10.0f, sh1);
	new ScrollAppearance <GUI_Window>(10.0f, sh1Hover);
	new ScrollAppearance <GUI_Window>(10.0f, sh2);
	new ScrollAppearance <GUI_Window>(10.0f, sh2Hover);
}

void MainMenu::pressDemoButton() {
	menu->hide();
	menu->freeze();
	title->hide();

	for (auto it : endingTitles) it->hide();
	titlesCounter->hide();

	stopScroll();
	timer = new Timer(2.0f, this, &MainMenu::startDemo);
	PLAYERCH->getCharacter()->requestAnimation("sheate", "idle");
	new Timer(1.0f, this, &MainMenu::fade);
}

void MainMenu::pressRendererButton() {
	menu->hide();
	menu->freeze();
	title->hide();

	for (auto it : endingTitles) it->hide();
	titlesCounter->hide();

	stopScroll();
	timer = new Timer(0.3f, this, &MainMenu::startRendererTest);
	new Timer(0.2f, this, &MainMenu::fade);
}

void MainMenu::stopScroll() {
	if (behaviour != nullptr && !behaviour->wasDestroyed()) behaviour->destroy();
	bScroll = false;
	PLAYERCH->getCharacter()->requestAnimation("idle_guitar");
}

void MainMenu::fade() {
	GOD->getRenderer()->overrideColor(DWHITE);
	new Timer(0.3f, this, &MainMenu::fade2);
	new Timer(0.6f, this, &MainMenu::fade3);
}
void MainMenu::fade2() {
	GOD->getRenderer()->overrideColor(GRAY);
}
void MainMenu::fade3() {
	hide();
	GOD->getRenderer()->resetOverride();
}

void MainMenu::startDemo() {
	script = new DemoScript();
}

void MainMenu::startRendererTest() {
}
