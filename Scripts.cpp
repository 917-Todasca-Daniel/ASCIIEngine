#include "Scripts.h"

#include "engine/GodClass.h"
#include "engine/ScreenObject.h"
#include "engine/ScreenObjectTable.h"

#include "Narration.h"
#include "AvatarCollection.h"
#include "Player.h"
#include "Encounter.h"
#include "Behaviours.h"

Player* ScriptUtilities::player = nullptr;
Narration* ScriptUtilities::narrator = nullptr;

template <typename container>
MapLayout::MapLayout(const container &arr, int map_x, int map_y) {
	scbt = new ScreenObjectTable("maptiles");
	
	for (auto it : arr) {
		map[it] = "square";
		objMap[it] = new ScreenObject(map_x + it.first * 4, map_y + it.second * 2, 500, "");
	}
}
MapLayout::~MapLayout() {
	for (auto it : objMap) it.second->destroy();
}

void MapLayout::setRoomShape(std::pair<int, int> pair, std::string shape) {
	map[pair] = shape;
}
void MapLayout::travelTo(std::pair<int, int> pair) {
	scbt->updateShape(objMap[pair], map[pair]);

	int x1 = pair.first, y1 = pair.second;
	for (auto it : objMap) {
		int x2 = it.first.first, y2 = it.first.second;
		if (it.second->data.size() > 0) continue;

		if (y1 == y2) {
			if (x2 == x1 + 1) {
				scbt->updateShape(it.second, "unknown_right");
			}
			else if (x2 == x1 - 1) {
				scbt->updateShape(it.second, "unknown_left");
			}
		}
		else
			if (x1 == x2) {
				if (y2 == y1 + 1) {
					scbt->updateShape(it.second, "unknown_down");
				}
				else if (y2 == y1 - 1) {
					scbt->updateShape(it.second, "unknown_up");
				}
			}
	}
}

void MapLayout::hide() {
	for (auto it : objMap)
		it.second->hide();
}

void MapLayout::show() {
	for (auto it : objMap)
		it.second->show();
}

ScriptGraph* ScriptUtilities::readFile(std::string filename) {
	ScriptGraph *graph = new ScriptGraph();
	std::ifstream file("scripts/" + filename + ".txt"); 
	std::string line, options, flag, ptext;
	int x = 0, y = 0;
	while (true) {
		std::getline(file, line);
		if (line[0] == '%') break;
		if (line[0] == '/') continue;

		std::vector <std::string> arr, tags;
		flag = ptext = "";

		if (line[0] == '*') {
			ptext = line.substr(1);
			std::getline(file, line);
		}
		if (line[0] == '!') {
			flag = line.substr(1);
			std::getline(file, line);
		}
		if (line[0] == '~') {
			arr = ScriptUtilities::parseOptions(line.substr(1));
			std::getline(file, line);
		}
		while (line[0] == '?') {
			tags.push_back(line.substr(1));
			std::getline(file, line);
		}
		if (line[0] == '[') {
			auto arr2 = split(line.substr(1), ' ');
			x = strToInt(arr2[0]);
			y = strToInt(arr2[1]);
			graph->tiles.insert({ x, y });
			std::getline(file, line);
		}

		if (line[0] == '#') line = line.substr(1);
		else line = "    " + line;
		graph->addNode({ std::pair <int, int> (x, y), -1, line, flag, tags, arr, ptext });
	}	file.close();
	return graph;
}

std::vector<std::string> ScriptUtilities::parseOptions(std::string optionsline) {
	return split(optionsline, '$');
}
void ScriptUtilities::readLinkFile(std::string filename, ScriptGraph*& graph) {
	std::ifstream file("scripts/links/" + filename + ".txt");
	std::string line;
	while (true) {
		std::getline(file, line);
		if (line[0] == '%') break;
		if (line[0] == '/') continue;
		if (line[0] == '&') {
			auto parent = line.substr(1);
			std::getline(file, line);
			int num = line[0] - '0';
			for (int i = 0; i < num; ++i) {
				std::getline(file, line);
				ParagraphConditionChecker* checker = nullptr;
				if (i == 0) checker = new EndcodeValueChecker <1>();
				if (i == 1) checker = new EndcodeValueChecker <2>();
				if (i == 2) checker = new EndcodeValueChecker <3>();
				if (i == 3) checker = new EndcodeValueChecker <4>();
				if (i == 4) checker = new EndcodeValueChecker <5>();
				if (i == 5) checker = new EndcodeValueChecker <6>();
				if (i == 6) checker = new EndcodeValueChecker <7>();
				if (i == 7) checker = new EndcodeValueChecker <8>();
				graph->addEdge(parent, line, checker, (i == 0));
			}
		}
		else {
			auto arr = split(line, '-');
			for (int i = 0; i + 1 < (int) arr.size(); ++i)
				graph->addEdge(arr[i], arr[i + 1], new EndcodeValueChecker <0>());
		}
	}	file.close();
}

bool ScriptUtilities::isPlaying(){ return NARRATOR->bPlay; }

bool ParagraphConditionChecker::check(const ParagraphNode& node) { return false; }

Script::Script(std::string filename, int map_x, int map_y) {
	graph = ScriptUtilities::readFile(filename);
	graph->play();
	bLoopPostcedence = true;
	map = new MapLayout(graph->getTiles(), map_x, map_y);
	setName("Script");

	for (auto it : getParagraph().tags)
		interpretTag(it);
}
void Script::destroy() {
	PrimitiveObject::destroy();
	delete map;
	delete graph;
	NARRATOR->hideWindow();
}
Script::~Script() { }

void Script::interpretTag(const std::string& tag) {
	if (tag == "scroll") {
		NARRATOR->scrollOpen();
	}	else
	if (tag == "maphide") {
		map->hide();
	}	else
	if (tag == "mapshow") {
		map->show();
	}	else
	if (tag == "gameover") {
		bOver = true;
	}	else 
	if (tag[0] == '{') NARRATOR->setAvatar(AvatarCollection::getAvatar(tag.substr(1)));
}

void Script::loop(float delta) {
	if (bDestroyed) return;
	PrimitiveObject::loop(delta);
	ParagraphNode& paragraph = getParagraph();

	if (!ScriptUtilities::isPlaying()) {
		int destination = -1;
		paragraph.endcode = NARRATOR->getSelection();
		for (auto& it : getLinks()) {
			if (it.second->check(paragraph)) {
				destination = it.first;
			}
		}

		if (destination != -1) {
			moveToNode(destination);
			if (map) map->travelTo(graph->getParagraph().mapTile);
			for (auto tag : getParagraph().tags)
				interpretTag(tag);
		}
	}
}

ScriptGraph::ScriptGraph() {}
ScriptGraph::~ScriptGraph() {
	for (int i = 0; i < (int) paragraphs.size(); ++i) {
		for (auto it : edges[i]) {
			if (it.second) delete it.second;
		}
	}
}
void ScriptGraph::play() {
	auto& x = getParagraph();
	NARRATOR->playText(x.description);
	if (x.promptText.size() > 0)
		NARRATOR->setPrompt(x.promptText);
	for (auto& it : x.options) {
		int left = it.find('*');
		if (left != it.npos) {
			int right = it.find('*', left + 1);
			std::string tag = it.substr(left + 1, right - left - 1);
			if (ScriptUtilities::player->checkTag(tag))
				NARRATOR->addOption("x");
			else
				NARRATOR->addOption(it.substr(right + 1));
		}
		else {
			NARRATOR->addOption(it);
		}
	}
}

DemoScript::DemoScript() : Script("demo") {
	ScriptUtilities::readLinkFile("demo_links", graph);
	NARRATOR->showWindow();
	map->setRoomShape({ 0, 0 }, "startsquare_left");
	bOver = false;
	ending = "";

	for (int i = 0; i < EVENT_COUNT; ++i)
		eventFlagState[i] = false;

	graph->addEdge("enterchoice1",	"room2sec", new EndcodeValueChecker<3>(this, &DemoScript::checkFlag			<ROOM2>), true);
	graph->addEdge("enterchoice1",	"room2",	new EndcodeValueChecker<3>(this, &DemoScript::checkMinusFlag	<ROOM2>));
	graph->addEdge("enter2",		"room2sec", new EndcodeValueChecker<3>(this, &DemoScript::checkFlag			<ROOM2>), true);
	graph->addEdge("enter2",		"room2",	new EndcodeValueChecker<3>(this, &DemoScript::checkMinusFlag	<ROOM2>));
	graph->addEdge("room2sec",		"armor2",	new EndcodeValueChecker<1>(this, &DemoScript::checkFlag			<ARMOR>), true);
	graph->addEdge("room2sec",		"armor",	new EndcodeValueChecker<1>(this, &DemoScript::checkMinusFlag	<ARMOR>));
	graph->addEdge("enter2",		"mirror2",	new EndcodeValueChecker<1>(this, &DemoScript::checkFlag			<MIRROR>), true);
	graph->addEdge("enter2",		"mirror1",	new EndcodeValueChecker<1>(this, &DemoScript::checkMinusFlag	<MIRROR>));
	graph->addEdge("enterchoice1",	"mirror2",	new EndcodeValueChecker<1>(this, &DemoScript::checkFlag			<MIRROR>), true);
	graph->addEdge("enterchoice1",	"mirror1",	new EndcodeValueChecker<1>(this, &DemoScript::checkMinusFlag	<MIRROR>));
	graph->addEdge("room2sec",		"cellar2",	new EndcodeValueChecker<4>(this, &DemoScript::checkFlag			<CELLAR>), true);
	graph->addEdge("room2sec",		"cellar",	new EndcodeValueChecker<4>(this, &DemoScript::checkMinusFlag	<CELLAR>));
	graph->addEdge("promptcellar",  "servant1",	new EndcodeValueChecker<1>(), true);
}

DemoScript::~DemoScript() {
}
void DemoScript::destroy() {
	Script::destroy();
}

void DemoScript::interpretTag(const std::string& tag) {
	Script::interpretTag(tag);
	if (tag.substr(0, 3) == "xyz") {
		std::string add = tag.substr(3);
		ScriptUtilities::player->tags.insert(add);
	}
	else if (tag == "runaway") ending = "The Runaway";
	else if (tag == "mirrortag") eventFlagState[MIRROR] = true;
	else if (tag == "dining") eventFlagState[ROOM2] = true;
	else if (tag == "armor") eventFlagState[ARMOR] = true;
	else if (tag == "cellar") eventFlagState[CELLAR] = true;
}

void DemoScript::loop(float delta) {
	if (bDestroyed) return;
	Script::loop(delta);
}