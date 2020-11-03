#ifndef CHARACTERS_H
#define CHARACTERS_H

#include "engine/PrimitiveObject.h"

#include "engine/stdc++.h"

enum DAMAGE_TYPE { WHITE_DMG = 0, GRAY_DMG, BLACK_DMG, RED_DMG, BLUE_DMG, YELLOW_DMG, GREEN_DMG, DAMAGE_TYPE_COUNT };
enum SENTIENCE	 { PLAYER = 0, OBJECT, AI, SENTIENCE_COUNT };

enum TARGET_ALIVESTATE { ALL_STATES = 0, ONLY_ALIVE, ONLY_DEAD };

class Character;	
class CombatMoveInterface;
class Spell;
class Attack;
class PrimitiveAIBrain;
struct CombatProfile;
template <typename type>
class CombatGridLineTraceBehaviour;

struct CombatProfile {
public:
	CombatProfile(std::string name = "");
	virtual ~CombatProfile();

	SENTIENCE sentience = PLAYER;

	inline std::string hpString() { return intToStr(HP) + "/" + intToStr(init->HP) + "HP"; }
	inline std::string apString() { return intToStr(AP) + "/" + intToStr(init->AP) + "AP"; }
	inline std::string mpString() { return intToStr(MP) + "/" + intToStr(init->MP) + "MP"; }

	std::string	name;
	int	HP;
	int MP, startMP, regenMP;
	int AP, startAP, regenAP;
	int fatigue = 0;
	int fatigueMP = 0;

	int power = 7;
	int def = 0;

	int lookRange;
	int attackRange;
	int	moveRange;

	bool bDead = false;
	bool bSkillcaster = true;
	bool bFirstTurn;

	inline bool isFlippedAttack() { return (attackTarget->grid_x < grid_x); }
	inline std::pair <int, int> getVulnerableTile() { return { grid_x + vulnerable_x, grid_y + vulnerable_y }; }
	inline std::pair <int, int> getBeginningTile() { return beginningTile; }
	inline std::pair <int, int> getTile() { return { grid_x, grid_y }; }

	void applyMovementCost(int value) { MP -= value; }
	void turnBeginEffects();
	void resetCooldowns();

	PrimitiveAIBrain* brainAI;
	Character *ch;
	CombatProfile *attackTarget;
	CombatProfile *init;

	CombatMoveInterface* lastMove;
	Spell* lastSpellcast;
	Attack* lastAttackcast;

	struct animationFlags {
		bool bMoving;
		int  movementDirection;
	}	prev, pres;

	std::vector <Attack*> attackArr;
	std::vector <Spell*> spellArr;

	std::pair <int, int> beginningTile;

	int grid_x, grid_y;
	int vulnerable_x, vulnerable_y;
};

struct BanditProfile : public CombatProfile {
public:
	BanditProfile(std::string name);
	virtual ~BanditProfile() {};
};
struct ImpProfile : public CombatProfile {
public:
	ImpProfile(std::string name);
	virtual ~ImpProfile() {};
};
struct MuteProfile : public CombatProfile {
public:
	MuteProfile(std::string name);
	virtual ~MuteProfile() {};
};
struct ArcherProfile : public CombatProfile {
public:
	ArcherProfile(std::string name);
	virtual ~ArcherProfile() {};
};
struct ObjectProfile : public CombatProfile {
	ObjectProfile(std::string name = "") { sentience = OBJECT; grid_x = 4; grid_y = 1; }
	std::string actualName;
};

class Character : public PrimitiveObject {
public:
	Character(std::string name= "");

	virtual void destroy() override;

	void setPosition(int x, int y);
	std::pair <int, int> getPosition() { return std::pair <int, int>(x, y); }

	virtual void requestAnimation(std::string name, std::string nextAnim = "");

	void createAttackVisualEffects(int dmg = 0);

	void show();
	void hide();
	void showVisual();
	void hideVisual();

	virtual std::string look() = 0; 

	void overrideRenderPlane();
	void resetPriority();

	void flipX();

	CombatProfile* cp;

protected:
	virtual ~Character();

	bool bHide;
	int  x, y;
	int  flipAxis = 15;

	virtual void loop(float delta) override;

	void setCombatProfile(CombatProfile* profile) { cp = profile; profile->ch = this; }

	std::vector <std::pair <class Animator*, class AnimatorTable*>> parts;
	void openTable(std::string name);

private:
	int visualIdx = 0;
	std::vector <std::pair <int, int>> dmgVisualOffsets = { {0, 0}, {-1, -1}, {1, 1}, {10, 2}, {11, 1}, {12, 4}, {1, 5}, {9, 1}, {9, 3}, {-1, 2}, {-2, 0}, {-2, 2}, {-3, 0} };

	friend class CombatGridLineTraceBehaviour <Character>;
};

//	AI

struct AIMove {
	class PrimitiveAIBrain* brain;
	AIMove(PrimitiveAIBrain* brain, float score) : brain(brain), score(score) {}
	float score;
	virtual void call() = 0;
};

struct AIAttack : public AIMove {
	AIAttack(CombatProfile* target, PrimitiveAIBrain* brain, float score, std::pair <int, int> tile);
	CombatProfile* target;
	std::pair <int, int> tile;
	virtual void call() override;
};

struct AISpell : public AIMove {
	AISpell(class Spell *spell, PrimitiveAIBrain* brain, float score, std::pair <int, int> tile);
	Spell* spell;
	std::pair <int, int> tile;
	virtual void call() override;
};

class PrimitiveAIBrain {
public:
	PrimitiveAIBrain(Character* ch);
	virtual ~PrimitiveAIBrain();

	void actionZero();
	void actionOne();
	void actionTwo();
	void actionMinus();

	inline void turnBeginEffects() {
		didNothingThisTurn = true;
	}

	int  front() { return actionQueue.front(); }
	void pop()   { actionQueue.pop(); }
	bool empty() { return actionQueue.empty(); }

	void runCombatAI();

	void targetSetup(int count, bool bDistinct, Spell* spell);

	void solveSelector(class InputSingleSelector* selector);

	Spell* getCandidateSpell() { return spell; }

protected:
	std::queue <int> actionQueue;
	
	bool defaultLastResortResponse;
	bool didNothingThisTurn;

	std::vector <AIMove*> moves;
	std::pair <int, int> tileAI;

	std::vector <CombatProfile*> targetArr;
	CombatProfile* target;
	Character* ch;
	Spell* spell;

	virtual bool makeLastResortDecision();
	virtual void doLastResortMovement();

	void resetData();

	std::pair <int, int> chooseAttackTile(std::map <std::pair <int, int>, std::pair <int, int>> map, std::set <std::pair <int, int>> forbidden);

	virtual std::pair <int, int> chooseAdvancementTile();
	virtual std::pair <int, int> chooseRetreatTile();

	void attackSetup(CombatProfile* target, std::pair <int, int> tile);
	void attackCheck();
	bool spellCheck(Spell* spell);
	void spellSetup(Spell* spell, std::pair <int, int> tile);

	virtual float parseEventArray(std::vector <class Event*> arr);
	virtual float computeTargetCandidateScore(CombatProfile* target, int mpCost, std::pair <int, int> tile);
	virtual float computeScore(class Spell* spell, std::pair <int, int> tile, int cost);

	virtual bool areEqual(std::pair <std::pair <int, int>, int> candidate, std::pair <std::pair <int, int>, int> old, Spell* spell);
	virtual bool isBetter(std::pair <std::pair <int, int>, int> candidate, std::pair <std::pair <int, int>, int> old, Spell* spell);
	virtual bool isEnemy(CombatProfile* profile);

private:
	int randIdx;
	std::vector <int> randArr;
	void recomputeRandArr() {
		randIdx = 0;
		randArr.clear();
		for (int i = 0; i < 100; ++i)
			randArr.push_back(Rand());
	}
	inline void resetRand() { randIdx = 0; }
	inline int getRand() {
		if (randIdx >= (int) randArr.size()) return Rand();
		return randArr[randIdx++];
	}

	friend struct AIAttack;
	friend struct AISpell;
};

class ImpBrain : public PrimitiveAIBrain {
public:
	ImpBrain(Character *ch);

protected:
	virtual float computeTargetCandidateScore(CombatProfile* target, int mpCost, std::pair <int, int> tile) override;
	virtual float computeScore(Spell* spell, std::pair <int, int> tile, int cost) override;

	int getAlliesAroundTile(std::pair <int, int> tile);

	virtual void doLastResortMovement() override;

	virtual bool areEqual(std::pair <std::pair <int, int>, int> candidate, std::pair <std::pair <int, int>, int> old, Spell* spell) override;
	virtual bool isBetter(std::pair <std::pair <int, int>, int> candidate, std::pair <std::pair <int, int>, int> old, Spell* spell) override;
};

class CHPaladin : public Character {
public:
	CHPaladin();
	std::string look() { return "What a handsome young lad."; };
	virtual void requestAnimation(std::string name, std::string nextAnim = "") override;
};

class TheMute : public Character {
public:
	TheMute();
	std::string look() { return "That's you, the mute bard!"; };
	virtual void requestAnimation(std::string name, std::string nextAnim = "") override;
};

class Bandit : public Character {
public:
	Bandit(std::string name);
	std::string look() { return "A bandit."; };
	virtual void requestAnimation(std::string name, std::string nextAnim = "") override;
};

class Imp : public Character {
public:
	Imp(std::string name);
	std::string look() { return "A bandit."; };
	virtual void requestAnimation(std::string request, std::string next = "") override;
};

class Archer : public Character {
public:
	Archer(std::string name);
	std::string look() { return "An archer."; }
	virtual void requestAnimation(std::string request, std::string next = "") override;
};

//	OBJECTS

class Object : public Character {
public:
	Object(std::string name);
	std::string look();
protected:
	std::string hiddenName;
};

//	REQUIREMENTS

class CombatMoveRequirement {
public:
	CombatMoveRequirement() {};
	virtual bool check(CombatProfile* profile) = 0;
	virtual void applyConsequences(CombatProfile *profile) = 0;
	virtual std::pair <std::string, std::string> descriptionGen() = 0;
};

class Cooldown : public CombatMoveRequirement {
public:
	Cooldown(int value, int charged = 0) : value(value), charged(charged-1) {}
	virtual bool check(CombatProfile* profile) {
		return charged >= value;
	}
	virtual void applyConsequences(CombatProfile* profile) {
		charged = 0;
	}	virtual std::pair <std::string, std::string> descriptionGen() { 
		std::string desc = intToStr(value);
		return { "Cooldown", desc };
	}
	int value, charged;
};

class MPCostRequirement : public CombatMoveRequirement {
public:
	MPCostRequirement(int value) : value(value) {}
	virtual bool check(CombatProfile* profile) {
		return (profile->MP >= value);
	}
	virtual void applyConsequences(CombatProfile* profile) {
		profile->MP -= value;
	}	virtual std::pair <std::string, std::string> descriptionGen() { return { "MP cost", intToStr(value) }; }
	int value;
};

class APCostRequirement : public CombatMoveRequirement {
public:
	APCostRequirement(int value) : value(value) {}
	virtual bool check(CombatProfile* profile) {
		return (profile->AP >= value);
	}
	virtual void applyConsequences(CombatProfile* profile) {
		profile->AP -= value;
	}	virtual std::pair <std::string, std::string> descriptionGen() { return { "AP cost", intToStr(value) }; }
	int value;
};

//	COMBAT INTERFACE

enum ValidityCode { VC_OK = 0, VC_NO_TARGETS = 1, VC_COUNT };

class CombatMoveInterface {
public:
	CombatMoveInterface() { minTargets = 1; source = nullptr; maxDescriptionLen = 0; bGlobal = false; color = RED; }
	virtual ~CombatMoveInterface() { for (auto requirement : requirements) delete requirement; }

	inline std::string getName() { return name; }
	virtual inline std::string descriptionHelper() { std::string ret; for (auto it : descriptionLine) ret += " " + it + "\n"; return ret; }
	virtual inline std::string descriptionGen() { return fantasy + '\n' + techDescription + "\n\n" + descriptionHelper(); }
	virtual std::string shortDescription() = 0;

	virtual bool checkRequirements(CombatProfile* profile) {
		source = profile;
		for (auto it : requirements) if (!it->check(profile)) return false;
		return true;
	}

	inline bool isGlobal() { return bGlobal; }

	virtual std::vector <CombatProfile*> targetsInRange(Character* caster, std::vector <Character*> characters) {
		std::vector <CombatProfile*> ans;
		CombatProfile *cp = caster->cp;
		std::set <std::pair <int, int>> targetPositions;
		for (auto it : rangeTiles(cp->grid_x, cp->grid_y)) targetPositions.insert(it);
		for (auto it : characters)
			if (checkTargetability(cp, it->cp) &&	
				it != cp->ch && targetPositions.find(it->cp->getTile()) != targetPositions.end())
				ans.push_back(it->cp);
		return ans;
	}

	virtual ValidityCode checkMoveValidity(Character* caster, std::vector <Character*> characters) {
		validTargets.clear();

		if (bGlobal) { for (auto it : characters) if (it != caster) validTargets.push_back(it); }
		else for (auto it : targetsInRange(caster, characters))
			validTargets.push_back(it->ch); 

		std::vector <Character*> cleaner;
		if (targetAliveState != ALL_STATES) for (auto it : validTargets) {
			if (targetAliveState == 1 + it->cp->bDead)
				cleaner.push_back(it);
		}	validTargets = cleaner;

		if ((int) validTargets.size() < minTargets) return VC_NO_TARGETS;
		return VC_OK;
	}

	void resetCooldown() {
		for (auto& it : requirements) {
			Cooldown* req = dynamic_cast <Cooldown*>(it);
			if (req != nullptr) req->charged = req->value;
		}
	}

	virtual void castMove(CombatProfile* profile) {}
	virtual void triggerEvents(std::vector <CombatProfile*> profiles) = 0;

	virtual std::vector <std::pair <int, int>> rangeTiles(int x, int y) {
		std::vector <std::pair <int, int>> ret;
		for (auto it : rangeTilesOffset) ret.push_back({ x + it.first, y + it.second });
		return ret;
	}
	virtual std::vector <std::pair <int, int>> effectTiles(int x, int y) {
		std::vector <std::pair <int, int>> ret;
		for (auto it : effectTilesOffset) ret.push_back({ x + it.first, y + it.second });
		return ret;
	}

	CombatProfile* source;
	std::vector <Character*> getValidTargets() { return validTargets; }

	COLOR getColor() { return color; }

protected:
	COLOR color; 

	int  minTargets;
	bool bGlobal;

	std::string name;
	std::string fantasy, techDescription;
	std::vector <Character*> validTargets;
	std::vector <CombatMoveRequirement*> requirements;

	int maxDescriptionLen;
	std::vector <std::string> descriptionLine;
	std::vector <std::pair <int, int>> rangeTilesOffset;
	std::vector <std::pair <int, int>> effectTilesOffset;

	TARGET_ALIVESTATE targetAliveState = ONLY_ALIVE;
	
	virtual bool checkTargetability(CombatProfile* src, CombatProfile* candidate) {
		return src->sentience != candidate->sentience;
	}

	void addDescription(std::string about, std::string description) {
		if (maxDescriptionLen <= (int) about.size()) {
			for (auto& it : descriptionLine) {
				int idx = 0;
				while (idx < (int) it.size() && it[idx] != ':') ++idx;
				while (idx < (int) it.size() && it[idx] != ' ') ++idx;
				int idx2 = 0;
				for (int i = 0; i < (int)about.size() - maxDescriptionLen; ++i) it.insert(idx++, " ");
			}
			maxDescriptionLen = about.size();
		}	descriptionLine.push_back(about + ": " + std::string(maxDescriptionLen - about.size(), ' ') + description);
	}
	
	void addRequirementsDescription() {
		for (auto& it : requirements) {
			auto des = it->descriptionGen();
			addDescription(des.first, des.second);
		}
	}

	void setTilesOffset(int range, std::vector <std::pair <int, int>>& offset) {
		offset.clear();
		for (int i = -range, j; i <= range; ++i)
			for (j = - (range - (std::max)(i, -i)); j <= range - (std::max)(i, -i); ++j)
				offset.push_back({ j, i });
	}
	void setLineTilesOffset(int range, std::vector <std::pair <int, int>>& offset) {
		offset.clear();
		for (int i = -range; i <= range; ++i)
			offset.push_back({ i, 0 });
	}

	void addRequirements(CombatMoveRequirement* requirement) { requirements.push_back(requirement); }
	template <class... other_arg> void addRequirements(CombatMoveRequirement* req, other_arg... other) {
		addRequirements(req); 
		addRequirements(other...);
	}
};

//	ATTACKS

class Attack : public CombatMoveInterface {
public:
	Attack() {}
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};

class DaggerAttack : public Attack {
public:
	DaggerAttack();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(3AP; 100% 1-hit gray damage)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class Claw : public Attack {
public:
	Claw();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(2AP; 100% 1-hit gray damage)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};

class ArrowAttack : public Attack {
public:
	ArrowAttack();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(3AP; 100% 1-hit gray damage)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};

//	SPELLS

class Spell : public CombatMoveInterface {
public:
	Spell() {}	
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
	void refresh() {
		for (auto& it : requirements) {
			Cooldown* cast = dynamic_cast <Cooldown*> (it);
			if (cast != nullptr && cast->charged < cast->value) cast->charged ++;
		}
	}
	std::string getBonusInfo() {
		std::string ans = " ";
		for (auto& it : requirements) {
			Cooldown* cast = dynamic_cast <Cooldown*> (it);
			if (cast != nullptr && cast->charged < cast->value) ans += "(" + intToStr(cast->charged) + ")";
		}	return ans;
	}
};

class ShadowTeleport : public Spell {
public:
	ShadowTeleport();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(2MP; teleport to enemy)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class ShadowReturn : public Spell {
public:
	ShadowReturn();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(2MP; teleport to starting tile)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class SelfSacrifice : public Spell {
public:
	SelfSacrifice();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(6AP next turn and 20%HP now; gain 6AP)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class Anticipation : public Spell {
public:
	Anticipation();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(no cost; leftover AP and MP carried next turn)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class SelfCombust : public Spell {
public:
	SelfCombust();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(100% HP; deal damage around)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class Assault : public Spell {
public:
	Assault();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(2MP and 3AP; move towards and 150% damage target)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class Hate : public Spell {
public:
	Hate();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(3AP; single-target damage based on missing HP)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class Backstab : public Spell {
public:
	Backstab();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(3AP; single-target 150% / vulnerability 400%)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class LifeGamble : public Spell {
public:
	LifeGamble();

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(3AP; single-target 100% / vulnerability 200%, bonus on kill)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);
};
class HealingFountain : public Spell {
public:
	HealingFountain(int value);

	virtual void castMove(CombatProfile* profile);
	virtual std::string shortDescription() { return "(3AP; single-target heal " + intToStr(value) + "%)"; }
	virtual void triggerEvents(std::vector <CombatProfile*> profiles);

	virtual bool checkTargetability(CombatProfile* src, CombatProfile* candidate) override {
		return src->sentience == candidate->sentience;
	}

private:
	int value;
};

#endif // CHARACTERS_H