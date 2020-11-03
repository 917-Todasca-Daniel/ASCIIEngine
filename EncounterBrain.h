#ifndef ENCOUNTERBRAIN_H
#define ENCOUNTERBRAIN_h

#include <string>
#include <vector>

struct CombatProfile;

class Parser {
public:
	Parser(std::string text);
	bool contains(std::string cmd);
	std::string getOriginalText() { return originalText; }
	std::string getErrorMessage() {
		if (originalText.empty()) return "Command was empty. What are you, a mute?";
		else return "Invalid command: '" + originalText + "'.";
	}

private:
	std::string originalText;
	std::vector <std::string> words;
};

class EncounterBrain {
public:
	EncounterBrain();
	virtual ~EncounterBrain();

	virtual void answer(CombatProfile* src, std::string command);
	virtual void answer(CombatProfile* src, Parser &parser);
	virtual void answer(CombatProfile* src, CombatProfile* target, class CombatMoveInterface* move);

protected:
	void runLookCommand(CombatProfile* src, Parser& parser);
	bool lookAtCharacter(class Character* ch);
	bool lookAtCharacterOnTile(int x, int y);

	friend class Debugger;
};

class ForestEncounterBrain : public EncounterBrain {
public:
	ForestEncounterBrain();
	virtual ~ForestEncounterBrain();

	virtual void answer(CombatProfile* src, std::string command) override;

	friend class Debugger;
};

#endif // ENCOUNTERBRAIN_H