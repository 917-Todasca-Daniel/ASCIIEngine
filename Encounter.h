#ifndef ENCOUNTER_H
#define ENCOUNTER_H

#include "EncounterLayout.h"

#define	ENCOUNTER Encounter::getInstance()
#define GRID	  Encounter::getInstance()->grid

struct CombatProfile;
struct ObjectProfile;
class InputSingleSelector;
class InputMultipleSelector;
class Character;
class Event;
class LogEntryType;
class EncounterBrain;
class ScreenObject;
class CombatMoveInterface;
class LoggerWindow;
class GUI_WindowsTable;
class GUI_Window;
class GUI_Menu;
class Narration;
class Timer;
class PromptAsker;
class EventTrigger;

class Encounter : public PrimitiveObject {
public:
	bool bSimMode;
	std::vector <Event*> simArr;
	void turnOnSim() { bSimMode = true; }
	void turnOffSim();

	void addErrorMessage(std::string message) { errorMessages.push_back(message); }

	Encounter(std::string grid_name, std::vector <Character*> characters);
	virtual void destroy() override;

	static Encounter* getInstance() { return instance; }

	std::vector <CombatProfile*> getProfiles() { return profiles; }
	std::vector <Character*>	 getCharacters() { return characters; }
	std::vector <ObjectProfile*> getObjects();

	struct logged_string {
		Event* event;
		LogEntryType* type;
	};	std::vector <logged_string> rawHistory;
	std::list <ScreenObject*> historyLog;

	//	event triggers are here checked
	template <typename log_type, typename event_type>
	void updateHistory(event_type* event);

	void logText(std::string text);

	inline void addEventTrigger(class EventTrigger *trigger) { triggers.push_back(trigger); }
	inline void addEventOnStack(Event* event)				 { 
		if (bSimMode) simArr.push_back(event);
		else eventStack.push_back(event); 
	}

	void combatMoveTargetRequestSetup(CombatMoveInterface *moveInterface, int count, bool bDistinct = true);

	bool checkValability(std::pair <int, int> pair);

	grid_info grid;

	void shakeVisualEffect(float time = 0.5f, float iter = 0.1f);

	//	AI support
	void pressAttack(CombatProfile *target);
	void pressSkills();
	void pressMovement();
	void pressEndTurn();
	void pressTile(std::pair <int, int> pair);

	void pauseAI() { bAllowAI = false; }
	void restartAI() { bAllowAI = true; }
	void addAIDelay(float time = 0.5f);

	void solveSelector(InputSingleSelector* selector);

protected:
	virtual ~Encounter();

	static Encounter* instance;
	EncounterBrain* brain;

	std::vector <CombatProfile*> profiles;
	std::vector <Character*>	 characters;

	enum MenuTypes { ACT = 0, POPUP, SPOPUP, MENUTYPECOUNT };

	virtual void	loop						(float delta) override;
	void			handleEventLogger			(float delta);
	void			handleErrorMessages			(float delta);
	void			handleCharactersAnimations	(float delta);
	void			updateUI					(float delta);

	bool    bPlayerVulnerability = false;
	bool	bPrompted = false;
	bool	bCanPrompt = true;
	bool	bPrintExtendedStats;
	bool	bAttackTargetRequest;
	bool	bAttacked;
	bool	bTilesSelection;
	bool	bInSelection;
	bool	bInMovement;
	bool	bStandby, bTurnEnded;
	bool	bDown, bUp, bLeft, bRight;
	bool	bPressedEscape;
	bool	bFreezeFight;
	bool	bAllowAI;

	int		count;
	int		pidx;
	float	pressTime, log_x, log_y;

	Timer*	aiTimer;

	ScreenObject*					arrow;
	ScreenObject*					hover_arrow;
	CombatMoveInterface*			request;
	CombatMoveInterface*			currentMove;
	CombatMoveInterface*			currentMoveOption;
	InputMultipleSelector*			multipleSelector;
	InputSingleSelector*			singleSelector;
	InputSingleSelector*			menuSelector;
	LoggerWindow*					eventLogger;
	Animator*						anim_hover_arrow;

	GUI_WindowsTable*	table;
	GUI_Window*			borders[MENUTYPECOUNT];
	GUI_Menu*			menus[MENUTYPECOUNT];
	GUI_Window*			descriptions;
	Narration*			endMessageWin;

	Controller* movementCtrl;
	Controller* escapeCheck;
	Controller* statsCtrl;
	Controller* logCtrl;
	Controller* endTurnCtrl;
	Controller* endScreenCtrl;
	std::vector <Controller*> controllers;

	void addController(Controller* controller) { controllers.push_back(controller); }
	void wakeUpControllers();

	std::string endMessage;

	std::vector <GridTile*>				movementTiles;
	std::vector <GridTile*>				selectedTiles;
	std::vector <std::pair <int, int>>	movementTilesInPair;
	std::vector <CombatProfile*>		selectionChoices;
	std::list <EventTrigger*>			triggers;
	std::vector <Event*>				eventStack;
	std::vector <ScreenObject*>			printers;
	std::vector <std::string>			printRequests;

	std::vector <std::string>						errorMessages;
	std::list <std::pair <ScreenObject*, float>>	errorMessagesObj;

	std::pair <int, int> seltile;

	PromptAsker		*commandPrompt = nullptr;
	PrimitiveObject *promptText;

	enum UIElement { HPBar = 0, HPBarMissing, HPPrint, HPDown, brackets, namePrint, nameDown, statsBorder, MAPrint,
		WPrint, APRegen, MPRegen, DWPrint, UI_COUNT };
	struct individualUIElements {
		individualUIElements();
		~individualUIElements() {}
		ScreenObject *elem[UI_COUNT];
	};	std::vector <individualUIElements*> UIObj;

	void requestTileSelection(int count, int range, int pivot_x, int pivot_y, bool bForbiddenRule = false);

	std::pair <int, int> centerPopUpHelper(int width);
	void centerPopUp();
	void centerHoverArrow(CombatProfile *hovered);

	PromptAsker* createPrompt();
	void sendPromptAnswer(std::string answer);
	void refreshPrompt() { bCanPrompt = true; }
	void runPromptTasks();
	void setupAttackInteraction(CombatProfile* src, CombatProfile* target, CombatMoveInterface* move);

	template <int x, int y> void moveTile();
	template <bool value>	void setLeft()	{ bLeft = value; };
	template <bool value>	void setRight() { bRight = value; };
	template <bool value>	void setUp()	{ bUp = value; };
	template <bool value>	void setDown()	{ bDown = value; };

	virtual bool checkWinCondition();
	virtual bool checkLoseCondition();
	virtual void checkEndConditions();
	virtual void freezeFight();

	void endFight();
	void killProfile(CombatProfile* profile);

	void flashHPBar(CombatProfile* profile);

	void runSelectionTasks();
	bool finalizeSelection(std::string name);
	bool finalizeSelection(std::vector <std::string> names);
	bool checkAttackSelection();
	bool checkSpellsSelection();
	void communicateSelection();
	void runSelections();
	void runTileSelection();

	void createSingleTargetInputSelector(std::vector <CombatProfile*> profiles);
	void createMultipleTargetInputSelector(std::vector <CombatProfile*> profiles, int count, bool bDistinct = true);
	void selectAttackSetup();

	void setupUI();
	void createMenus();
	void internal_addButtonRoutine(MenuTypes type, int idx, std::string(*func)(), std::string(*hfunc)(), void (Encounter::* buttonPress)(), Color base = Color(), Color hovered = Color());
	void setupPlayerTurn();
	void endPlayerTurn();
	
	void selectTile();
	void toggleLogVisibility();
	void statsWindowToggle() { bPrintExtendedStats = !bPrintExtendedStats; }

	void runStack();

	void manageInputControl();
	void manageEndscreen();
	void manageRenderPriorities();
	void manageVulnerabilitiesRendering();

	void pressEscape() { bPressedEscape = true; };
	bool checkEscapePress();
	void escapeAction();
	void attackPress();
	void spellPress();
	void movePress();
	void commandPress();
	void endTurnPress();

	std::vector <Character*> currentMoveTargets();
	std::vector <CombatProfile*> currentMoveProfiles();
	std::vector <CombatProfile*> targetsInRange(CombatProfile* source);
	std::vector <std::pair <int, int>> getTilesInRange(int x, int y, int range);
	CombatProfile* getProfileByName(std::string name);
	CombatProfile* currentProfile();

	void runEndTurnTasks();
	void castAttack(CombatProfile* target);

	std::string descriptionGen();

	void runAITasks();

	friend class FixedDamageEvent;
	friend class DeathEvent;
	friend class TrueScallingMAXHPDamageEvent;
	friend class VulnerabilityConsumptionEvent;

	friend class Debugger;
	friend class GodClass;
};

template<int x, int y>
inline void Encounter::moveTile() {
	int x2, y2;
	x2 = this->seltile.first + x; y2 = this->seltile.second + y;
	bool bFound = false;
	for (auto it : movementTilesInPair) if (it == std::pair <int, int>(x2, y2)) bFound = true;
	if (LAYOUTS->selectTile(grid, { x2, y2 }) != grid.tiles.size() && bFound) this->seltile = { x2, y2 };
}

//	EVENTS MANAGING STARTS HERE

class Event {
public:
	bool bPositionChange = false;
	Event();
	std::string bonus;
	CombatProfile *src, *dest;
	virtual void call();
};
class DamageEvent : public Event {};
class EventTrigger {
public:
	EventTrigger(Event *event);

	union {
		class EventGenerator *eventGen;
		Event* event;
	}	effect;
	std::vector <CombatProfile*> profiles;

	virtual bool check(const std::vector <Encounter::logged_string> &rawHistory);
	virtual void fire();	

	bool bFired;

	friend class Debugger;
	friend class God;
};
class LogEntryType {
public:
	LogEntryType() {}
	virtual std::string genDescription(Event* event) = 0;
};

template <typename log_type, typename event_type>
void Encounter::updateHistory(event_type* event) {
	rawHistory.push_back({ event, new log_type() });
	for (auto trigger = triggers.begin(); trigger != triggers.end();)
		if ((*trigger)->check(rawHistory)) {
			(*trigger)->fire();
			auto copy = *trigger;
			trigger = triggers.erase(trigger);
			delete copy;
		}	else ++trigger;
	std::string str;
	if ((str = rawHistory.back().type->genDescription(rawHistory.back().event)) != "skip")
		printRequests.push_back(str);
}

class SpellcastLog : public LogEntryType {
public:
	SpellcastLog() {};
	virtual std::string genDescription(Event* event) { return event->src->name + " has cast " + event->src->lastSpellcast->getName() + " " + event->src->lastSpellcast->shortDescription() + "."; }
};
class AttackcastLog : public LogEntryType {
public:
	AttackcastLog() {};
	virtual std::string genDescription(Event* event) { return event->src->name + " has used " + event->src->lastAttackcast->getName() + " " + event->src->lastAttackcast->shortDescription() + "."; }
};
class MovementLog : public LogEntryType {
public:
	MovementLog() {};
	virtual std::string genDescription(Event* event) { return "skip"; }
};
class TriggerChecker : public LogEntryType {
public:
	TriggerChecker() {}
	virtual std::string genDescription(Event* event) { return "skip"; }
};
class TurnBeginLog : public LogEntryType {
public:
	TurnBeginLog() {}
	virtual std::string genDescription(Event* event) { return event->src->name + "'s turn has begun."; }
};
class TurnEndLog : public LogEntryType {
public:
	TurnEndLog() {}
	virtual std::string genDescription(Event* event) { return "skip"; }
};
class GlobalTurnLoop : public LogEntryType {
public:
	GlobalTurnLoop() {};
	static int counter;
	virtual std::string genDescription(Event* event) { return "Turn " + intToStr(++counter) + " begins!"; }
};
class CombatBeginLog : public LogEntryType {
public:
	CombatBeginLog() {};
	virtual std::string genDescription(Event* event) { return "Combat has started!"; }
};
class MoveCritLET : public LogEntryType {
public:
	MoveCritLET() {};
	virtual std::string genDescription(Event* event);
};

class TeleportLET : public LogEntryType {
public:
	TeleportLET() {}
	virtual std::string genDescription(Event* event);
};
class TeleportEvent : public Event {
public:
	TeleportEvent(std::pair <int, int> tile) { this->tile = tile; bPositionChange = true; }
	virtual void call() override { this->src->grid_x = tile.first; this->src->grid_y = tile.second; Encounter::getInstance()->updateHistory <TeleportLET>(this);}

private:
	std::pair <int, int> tile;
};

class FixedDamageLET : public LogEntryType {
public:
	FixedDamageLET() {}
	virtual std::string genDescription(Event* event);
};
class FixedDamageEvent : public DamageEvent {
public:
	FixedDamageEvent(int value) { this->value = value; }
	inline int getValue() { return value; }
	virtual void call() override { 
		value = value * src->power * (100 - dest->def) / 10000;
		float proc = ((float)(Rand()) / RAND_MAX_VALUE) - 0.3f; proc *= 0.2f;
		value += (int) (proc * value);
		this->dest->HP -= value;
		this->dest->ch->createAttackVisualEffects(value);
		Encounter::getInstance()->flashHPBar(this->dest);
		Encounter::getInstance()->updateHistory <FixedDamageLET>(this); 
	}

private:	
	int value;
};

class HealEventLET : public LogEntryType {
public:
	HealEventLET() {}
	virtual std::string genDescription(Event* event);
};
class HealEvent : public Event {
public:
	HealEvent(int value) { this->value = value; }
	virtual void call() override { 
		value = value * dest->init->HP;
		if (dest->HP + value > dest->init->HP) value = dest->init->HP - dest->HP;
		dest->HP += value;
		Encounter::getInstance()->updateHistory <HealEventLET>(this); 
	}

	inline int getValue() { return value; }

private:
	int value;
};

class TrueScallingMAXHPDamageLET : public LogEntryType {
public:
	TrueScallingMAXHPDamageLET() {}
	virtual std::string genDescription(Event* event);
};
class TrueScallingMAXHPDamageEvent : public DamageEvent {
public:
	TrueScallingMAXHPDamageEvent(int value) { this->value = value; }
	inline int getValue() { return value; }
	virtual void call() override { 
		value = value * dest->init->HP / 100; this->dest->HP -= value;
		this->dest->ch->createAttackVisualEffects(value);
		Encounter::getInstance()->flashHPBar(this->dest);
		Encounter::getInstance()->updateHistory <TrueScallingMAXHPDamageLET>(this); 
	}

private:
	int value;
};

class VulnerabilityConsumptionEvent : public Event {
public:
	VulnerabilityConsumptionEvent() { }
	virtual void call() override {
		LAYOUTS->paintTile(Encounter::getInstance()->grid, dest->getVulnerableTile());
		dest->vulnerable_x = -100;
		dest->vulnerable_y = -100;
	}
};

class Retaliate : public EventTrigger {
public:
	Retaliate(Event *event) : EventTrigger(event) {}

	virtual bool check(const std::vector <Encounter::logged_string>& rawHistory) override {
		if (rawHistory.empty()) return false;
		auto& it = rawHistory.back();
		DamageEvent* cast = dynamic_cast <DamageEvent*> (it.event);
		if (cast && cast->dest == profiles[0]) {
			effect.event->src = profiles[0];
			effect.event->dest = cast->src;
			return true;
		}	return false;
	}
};

class DeathEventLET : public LogEntryType {
public:
	DeathEventLET() {}
	virtual std::string genDescription(Event* event);
};
class DeathEvent : public Event {
public:
	DeathEvent(CombatProfile* profile) : Event() { this->dest = profile; }
	virtual void call() override {
		Encounter::getInstance()->updateHistory <DeathEventLET>(this);
		Encounter::getInstance()->killProfile(dest);
	}
};
class DeathDetection : public EventTrigger {
public:
	DeathDetection(CombatProfile* target) : target(target), EventTrigger(new DeathEvent(target)) {}
	CombatProfile* target;

	virtual bool check(const std::vector <Encounter::logged_string>& rawHistory) override {
		if (target->HP <= 0) return true;
		return false;
	}
};

class KillRewardLET : public LogEntryType {
public:
	KillRewardLET() {}
	virtual std::string genDescription(Event* event);
};
class KillRewardEvent : public Event {
public:
	KillRewardEvent(CombatProfile* profile) : Event() { this->src = profile; }
	virtual void call() override {
		src->AP += 6;
		src->MP += 4;
		src->resetCooldowns();
		Encounter::getInstance()->updateHistory <KillRewardLET>(this);
	}
};
class KillRewardTrigger : public EventTrigger {
public:
	KillRewardTrigger(CombatProfile* target, CombatProfile* benefitor) : target(target), EventTrigger(new KillRewardEvent(benefitor)) { bState = (target->HP <= 0); }
	CombatProfile* target;
	bool bState;

	virtual bool check(const std::vector <Encounter::logged_string>& rawHistory) override {
		if (rawHistory.empty()) return false;
		auto& it = rawHistory.back();
		DamageEvent* cast = dynamic_cast <DamageEvent*> (it.event);
		if (cast != nullptr) {
			if (bState) {
				effect.event = nullptr;
				return true;
			}
			if (target->HP <= 0) {
				return true;
			}

			effect.event = nullptr;
			return true;
		}	return false;
	}
};

class ForestBanditsFight : public Encounter {
public:
	ForestBanditsFight(std::vector <Character*> characters);
};
class DiningFight : public Encounter {
public:
	DiningFight(std::vector <Character*> characters);
};

#endif // ENCOUNTER_H