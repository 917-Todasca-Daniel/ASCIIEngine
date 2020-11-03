#ifndef PLAYER_H
#define PLAYER_H

#include <vector>
#include <set>
#include <string>

class Character;

class Player {
public:
	Player();

	Character*& getCharacter() { return party[0]; }
	std::vector <Character*> getParty() { return party; }

	std::set <std::string> tags;
	bool checkTag(std::string str) {
		if (tags.find(str) == tags.end())
			return false;
		return true;
	}

private:
	std::vector <Character*> party;

	friend class Debugger;
};

#endif // PLAYER_H