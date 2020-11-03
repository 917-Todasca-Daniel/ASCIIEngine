#include "EncounterBrain.h"

#include "Encounter.h"
#include "Characters.h"

#include "engine/stdc++.h"

Parser::Parser(std::string text) {
	auto arr = split(text, ' ');
	for (int len = 0, i = 0; i < (int) arr.size() && words.size() < 3; ++i) {
		if (arr[i].size() > 1) words.push_back(arr[i]);
	}	originalText = text;
}

bool Parser::contains(std::string cmd) {
	for (auto& it : words) if (it == cmd) return true;;
	return false;
}

#define	ENCOUNTER	Encounter::getInstance()

EncounterBrain::EncounterBrain() {}
EncounterBrain::~EncounterBrain() {}

void EncounterBrain::answer(CombatProfile* src, CombatProfile* target, CombatMoveInterface* move) {}
void EncounterBrain::answer(CombatProfile* src, std::string command) {
	Parser parser(command);
	answer(src, parser);
}
void EncounterBrain::answer(CombatProfile* src, Parser &parser) {
	if (parser.contains("look") || parser.contains("inspect")) runLookCommand(src, parser);
	else ENCOUNTER->logText(parser.getErrorMessage());
}

void EncounterBrain::runLookCommand(CombatProfile *src, Parser &parser) {
	bool bFound = false;

	if (parser.contains("around")) {
		int range = 1;
		int src_x = src->grid_x;
		int src_y = src->grid_y;
		for (int y = -range; y <= range; ++y) {
			int bound = range - abs(y);
			for (int x = -bound; x <= bound; ++x) {
				if (x == 0 && y == 0) continue;
				bFound |= lookAtCharacterOnTile(x+src_x, y+src_y);
			}
		}
	}
	else {
		int x = 0, y = 0;
		if (parser.contains("left") || parser.contains("west")) {
			x = -1;
		}	else
		if (parser.contains("right") || parser.contains("east")) {
			x = 1;
		}	else
		if (parser.contains("up") || parser.contains("north")) {
			y = -1;
		}	else
		if (parser.contains("down") || parser.contains("south")) {
			y = 1;
		}	
		
		if (x + y != 0) {
			x += src->grid_x; y += src->grid_y;
			bFound |= lookAtCharacterOnTile(x, y);
		}
		else {
			bFound = true;
			ENCOUNTER->logText("Nothing to be seen - get more specific!");
		}
	}

	if (!bFound) ENCOUNTER->logText("Nothing to be seen - get closer to what you want to inspect!");
}

bool EncounterBrain::lookAtCharacter(Character* ch) {
	ENCOUNTER->logText(ch->look()); return true; 
}
bool EncounterBrain::lookAtCharacterOnTile(int x, int y) {
	for (auto ch : ENCOUNTER->getCharacters())
		if (ch->cp->grid_x == x && ch->cp->grid_y == y)
			return lookAtCharacter(ch);
	return false;
}


ForestEncounterBrain::ForestEncounterBrain() {}
ForestEncounterBrain::~ForestEncounterBrain() {}

void ForestEncounterBrain::answer(CombatProfile* src, std::string command) {
	Parser parser(command);
	if (parser.contains("climb")) {
		int x = src->grid_x;
		int y = src->grid_y;
		if (x == 2 && y == 0) {
			ForestBanditsFight* cast = dynamic_cast <ForestBanditsFight*> (ENCOUNTER);
			ENCOUNTER->logText("You climb the nearby tree");
		}
		else {
			ENCOUNTER->logText("Climb? Interesting idea. But you're not quite in a place you could climb anything - try somewhere else.");
		}
	}
	else {
		EncounterBrain::answer(src, parser);
	}
}


