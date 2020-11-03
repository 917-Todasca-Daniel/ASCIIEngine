#include "Player.h"

#include "Characters.h"

Player::Player() {
	party.push_back(new TheMute());
	party.push_back(new Archer("Archer"));
}
