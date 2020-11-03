#include "Characters.h"

#include "engine/Animator.h"
#include "engine/AnimatorTable.h"
#include "Behaviours.h"
#include "engine/Menu.h"
#include "engine/Debugger.h"

#include "Encounter.h"

CombatProfile::CombatProfile(std::string name) : name(name) {
	AP = regenAP = startAP = 0;
	MP = regenMP = startMP = 0;
	moveRange = 4;
	HP = 13; lastAttackcast = nullptr; lastSpellcast = nullptr;
	attackRange = 1;
	attackTarget = nullptr;
	prev.bMoving = pres.bMoving = false;
	brainAI = nullptr;
	vulnerable_x = vulnerable_y = 0;
	lookRange = 1;
	lastMove = nullptr;
	init = nullptr;
	grid_x = grid_y = 0;
	ch = nullptr;
	bFirstTurn = true;
}

CombatProfile::~CombatProfile() {
	if (init)	 delete init;
	if (brainAI) delete brainAI;
	for (auto it : attackArr) delete it;
	for (auto it : spellArr)  delete it;
}

void CombatProfile::turnBeginEffects() {
	if (brainAI) brainAI->turnBeginEffects(); 

	beginningTile = getTile();
	if (!bFirstTurn) {
		AP = ((AP += regenAP) >= init->AP ? init->AP : AP);
		MP = ((MP += regenMP) >= init->MP ? init->MP : MP);
		AP -= fatigue;
		MP -= fatigueMP;
		fatigue = 0;
		fatigueMP = 0;
	}	bFirstTurn = false;

	for (auto& it : spellArr) it->refresh();
}

void CombatProfile::resetCooldowns() {
	for (auto& it : spellArr) { it->resetCooldown(); }
}

BanditProfile::BanditProfile(std::string name) : CombatProfile(name) {
	attackArr.push_back(new DaggerAttack());
	spellArr.push_back(new LifeGamble());
	sentience = AI;
	regenAP = 6;
	AP = 6;
	startAP = 6;
	MP = 80;
	HP = 20;
	regenMP = 8;
	startMP = 80;
	attackRange = 1;
}
MuteProfile::MuteProfile(std::string name) : CombatProfile(name) {
	sentience = PLAYER;
	attackArr.push_back(new DaggerAttack());

	spellArr.push_back(new ShadowTeleport());
	spellArr.push_back(new ShadowReturn());
	spellArr.push_back(new SelfSacrifice());
	
	spellArr.push_back(new Hate());
	spellArr.push_back(new Backstab());
	spellArr.push_back(new LifeGamble());
	
	attackRange = 1;
	regenAP = 6;
	AP = 15;
	startAP = 6;
	MP = 8;
	HP = 200;
	regenMP = 8;
	startMP = 8;
}
ArcherProfile::ArcherProfile(std::string name) : CombatProfile(name) {
	sentience = PLAYER;
	attackArr.push_back(new ArrowAttack());
	spellArr.push_back(new HealingFountain(35));
	attackRange = 5;
	regenAP = 3;
	AP = 90;
	startAP = 60;
	MP = 80;
	HP = 150;
	regenMP = 4;
	startMP = 60;
}
ImpProfile::ImpProfile(std::string name) : CombatProfile(name) {
	sentience = AI;
	attackArr.push_back(new Claw());
	spellArr.push_back(new Assault());
	spellArr.push_back(new SelfCombust());
	attackRange = 1;
	regenAP = 3;
	AP = 6;
	startAP = 6;
	MP = 80;
	HP = 10;
	regenMP = 4;
	startMP = 60;
}

Character::Character(std::string name) {
	setName("Character");
	x = y = 0;
	bHide = false;
	cp = nullptr;
}

Character::~Character() {
	for (auto it : parts) {
		if (it.second) delete it.second;
	}
}
void Character::destroy() {
	PrimitiveObject::destroy();
	for (auto it : parts) {
		if (it.first) it.first->destroy();
	}
}

void Character::loop(float delta) {
	PrimitiveObject::loop(delta);

	random_shuffle(begin(dmgVisualOffsets), end(dmgVisualOffsets));

	visualIdx = 0;

	if (bHide) hideVisual();
	else showVisual();

	if (cp) {
		int val = (cp->prev.bMoving ^ cp->pres.bMoving) * (cp->pres.bMoving + 1);
		if (val == 1)		requestAnimation("idle");
		else if (val == 2) {
			std::string str = "walk" + intToStr(cp->pres.movementDirection);
			requestAnimation(str, str);
		}

		cp->prev = cp->pres;
	}
}

void Character::requestAnimation(std::string name, std::string nextAnim) {
	if (ENCOUNTER && ENCOUNTER->bSimMode == true) return;

	for (auto it : parts) {
		it.first->reset(), it.second->animate(it.first, name), it.first->play();
		it.first->setAnimTable(it.second);
	}
	if (name == "attack") {
		if (cp->isFlippedAttack()) {
			for (auto it : parts)
				it.first->flipX(flipAxis);
		}
	}
	if (name == "idle") nextAnim = "idle";
	if (nextAnim != "") {
		for (auto it : parts) it.first->setNextAnim(nextAnim);
	}
	else {
		for (auto it : parts) it.first->setAnimTable(nullptr);
	}
	for (auto it : parts) it.first->setStartFrame(0);
}

void Character::createAttackVisualEffects(int dmg) {
	if (dmg != 0) {
		new FlashBehaviour <Character>(5, 0.1f, 0.1f, this);
		new TextFloatingPopup("-" + intToStr(dmg), 1.0f + rand() % 10 * 0.1f, 8.0f + rand() % 5 * 0.1f, x + dmgVisualOffsets[visualIdx].first, y + dmgVisualOffsets[visualIdx].second);
		visualIdx = (visualIdx + 1) % dmgVisualOffsets.size();
	}
}

void Character::show() { bHide = false; showVisual(); }
void Character::hide() { bHide = true;  hideVisual(); }

void Character::showVisual() {
	for (auto it : parts) it.first->show();
}
void Character::hideVisual() {
	for (auto it : parts) it.first->hide();
}
void Character::setPosition(int x, int y) {
	this->x = x, this->y = y;
	for (auto it : parts) it.first->setPosition(x, y);
}

void Character::openTable(std::string name) {
	parts.push_back({ new Animator("dummy", 10, false), new AnimatorTable(name) });
	parts.back().first->setPosition(-1000, -1000);
	parts.back().first->setName("part");
}

void Character::overrideRenderPlane() {
	for (auto& it : parts) it.first->overrideRenderPlane(20);
}
void Character::resetPriority() {
	for (auto& it : parts) it.first->overrideRenderPlane(10);
}

void Character::flipX() {
	for (auto& it : parts) {
		it.first->flipX(flipAxis);
	}
}

//	SPECIFICS

TheMute::TheMute() : Character("The Mute Bard") {
	setCombatProfile(new MuteProfile("The Mute"));
	openTable("themute");
	requestAnimation("idle");
	show();
	setName("mute");
	for (auto it : parts)
		it.first->setName("muteAnim");
}
void TheMute::requestAnimation(std::string name, std::string nextAnim) {
	Character::requestAnimation(name, nextAnim);
}

Bandit::Bandit(std::string name) : Character(name) {
	setCombatProfile(new BanditProfile(name));
	openTable("bandit");
	requestAnimation("idle");
	show();
	cp->brainAI = new PrimitiveAIBrain(this);
	setName("bnd");
	for (auto it : parts)
		it.first->setName("bndAnim");
}
void Bandit::requestAnimation(std::string name, std::string nextAnim) {
	Character::requestAnimation(name, nextAnim);
}

Imp::Imp(std::string name) : Character(name) {
	setCombatProfile(new ImpProfile(name));
	openTable("imp");
	requestAnimation("idle");
	show();
	cp->brainAI = new ImpBrain(this);
	setName("imp");
	for (auto it : parts)
		it.first->setName("impAnim");
}

void Imp::requestAnimation(std::string request, std::string next) {
	if (request == "walk0") {
		Character::requestAnimation("walk1", "");
		for (auto it : parts)
			it.first->flipX(flipAxis);
	}
	else {
		Character::requestAnimation(request, next);
	}
}

std::string vulnerable_description = "marked by x; consumed after use";

CHPaladin::CHPaladin() : Character("Paladin") {
	cp = new CombatProfile("Paladin");
	openTable("paladin");
	requestAnimation("idle");
	show();
}
void CHPaladin::requestAnimation(std::string name, std::string nextAnim) {
	Character::requestAnimation(name);
}

Archer::Archer(std::string name) : Character(name) {
	setCombatProfile(new ArcherProfile(name));
	openTable("archer");
	requestAnimation("idle");
	show();
	setName("archer");
	for (auto it : parts)
		it.first->setName("archerAnim");
}

void Archer::requestAnimation(std::string request, std::string next) {
	if (request == "walk1") {
		Character::requestAnimation("walk0", "");
		for (auto it : parts)
			it.first->flipX(flipAxis);
	}
	else {
		Character::requestAnimation(request, next);
		if (request == "cast") {
			for (auto it : parts)
				it.first->setStartFrame(3);
		}
	}
}

//	OBJECTS
Object::Object(std::string name) : Character(name) {
	cp = new ObjectProfile(name);
	openTable("paladin");
	requestAnimation("idle");
	show();
	hiddenName = name;
}
std::string Object::look() {
	cp->name = hiddenName;
	std::string details = "You look at the still statue of the paladin. It's beautiful, but it's on your way.";
	return details;
}

//	SPELLS

ShadowTeleport::ShadowTeleport() {
	name = "Shadow Teleport";
	bGlobal = true;
	addRequirements(new MPCostRequirement(2), new Cooldown(3, 3));
	setTilesOffset(0, effectTilesOffset);
	fantasy = " Shadow Teleport\n\n    There's always a shadow in your back. But whose is that right now?\n";
	addDescription("Cast Range", "global");
	techDescription = "    Teleport to the VULNERABILITY tile of an enemy. If that tile is unavailable, teleport to a random adjacent available one. If no adjacent tile is available, you cannot cast this spell.";
	addRequirementsDescription();
}
void ShadowTeleport::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	ENCOUNTER->combatMoveTargetRequestSetup(this, 1);
}
void ShadowTeleport::triggerEvents(std::vector<CombatProfile*> profiles) {
	CombatProfile* target = profiles[0];
	std::pair <int, int> p(-1, -1);
	if (ENCOUNTER->checkValability(target->getVulnerableTile())) {
		p = target->getVulnerableTile();
	}
	else {
		std::pair <int, int> dxy[] = { {-1, 0}, {1, 0}, {0, 1}, {0, -1} };
		random_shuffle(dxy, dxy + 4);
		for (int i = 0; i < 4; ++i) {
			std::pair <int, int> x = { dxy[i].first + target->grid_x, dxy[i].second + target->grid_y };
			if (ENCOUNTER->checkValability(x)) p = x, i = 5;
		}
	}

	if (p.first == -1) {
		ENCOUNTER->addErrorMessage("Cannot teleport, no available adjacent tiles!");
	}
	else {
		if (ENCOUNTER->bSimMode == false) {
			Animator* anim = new Animator("teleportpart", source->ch->getPosition().first, source->ch->getPosition().second, 0, true, true);
			anim->setName("tpAnim");
		}
		source->ch->requestAnimation("kneelup", "idle");
		Spell::triggerEvents(profiles);
		TeleportEvent* event = new TeleportEvent(p);
		event->src = source;
		ENCOUNTER->addEventOnStack(event);
	}
}

ShadowReturn::ShadowReturn() {
	name = "Shadow Return";
	bGlobal = true;
	addRequirements(new MPCostRequirement(2), new Cooldown(3, 3));
	setTilesOffset(0, effectTilesOffset);
	fantasy = " Shadow Return\n\n    We all wish we could sometimes just go back to where we began. Guess what - you can do that!\n";
	addDescription("Cast Range", "global");
	techDescription = "    Teleport to the tile you were on at the start of your turn. If that tile is unavailable, you cannot cast this spell.";
	addRequirementsDescription();
}
void ShadowReturn::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	triggerEvents(std::vector <CombatProfile*>());
}
void ShadowReturn::triggerEvents(std::vector<CombatProfile*> profiles) {
	CombatProfile* target = source;
	std::pair <int, int> p(-1, -1);
	if (ENCOUNTER->checkValability(target->getBeginningTile())) {
		p = target->getBeginningTile();
	}

	if (p.first == -1) {
		ENCOUNTER->addErrorMessage("Cannot teleport, your starting tile is unavailable!");
	}
	else {
		if (ENCOUNTER->bSimMode == false) {
			Animator* anim = new Animator("teleportpart", source->ch->getPosition().first, source->ch->getPosition().second, 0, true, true);
			anim->setName("tpAnim");
		}
		source->ch->requestAnimation("kneelup", "idle");
		Spell::triggerEvents(profiles);
		TeleportEvent* event = new TeleportEvent(p);
		event->src = source;
		ENCOUNTER->addEventOnStack(event);
	}
}

SelfSacrifice::SelfSacrifice() {
	name = "Self Sacrifice";
	bGlobal = true;
	addRequirements(new Cooldown(1, 1));
	setTilesOffset(0, effectTilesOffset);
	fantasy = " Self Sacrifice\n\n    Determination can be often found in pain.\n";
	addDescription("HP Cost", "15%HP");
	techDescription = "    Lose 20% max HP from your current HP to gain 6AP this turn. You will have 6AP less regen next turn.";
	minTargets = 0;
	addRequirementsDescription();
}
void SelfSacrifice::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	std::vector<CombatProfile*> profiles;
	if (!ENCOUNTER->bSimMode)
		triggerEvents(profiles);
}
void SelfSacrifice::triggerEvents(std::vector<CombatProfile*> profiles) {
	Spell::triggerEvents(profiles);
	source->ch->requestAnimation("sacrifice", "idle");
	TrueScallingMAXHPDamageEvent* damageEvent = new TrueScallingMAXHPDamageEvent (20);
	damageEvent->src = source;
	damageEvent->dest = source;
	source->AP += 6;
	source->fatigue += 6;
	ENCOUNTER->addEventOnStack(damageEvent);
}

Anticipation::Anticipation() {
	name = "Anticipation";
	bGlobal = true;
	addRequirements(new Cooldown(5, 5));
	setTilesOffset(0, effectTilesOffset);
	fantasy = " Anticipation\n\n    A hunter must choose when to hunt.\n";
	addDescription("AP Cost", "none");
	techDescription = "    Set your AP and MP to zero. Those values get carried for next turn, carried above the threshold.";
	minTargets = 0;
	addRequirementsDescription();
}
void Anticipation::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	std::vector<CombatProfile*> profiles;
	triggerEvents(profiles);
}
void Anticipation::triggerEvents(std::vector<CombatProfile*> profiles) {
	Spell::triggerEvents(profiles);
	source->fatigue = -source->AP;
	source->AP = 0;
	source->fatigueMP = -source->MP;
	source->MP = 0;
}

SelfCombust::SelfCombust() {
	name = "Self Combust";
	addRequirements(new Cooldown(5, 5));
	setTilesOffset(0, effectTilesOffset);
	setTilesOffset(4, rangeTilesOffset);
	fantasy = " Self Combust\n\n    When life goes wrong... well, end it.\n";
	addDescription("AP Cost", "none");
	techDescription = "    Set your HP to zero. Deal damage around you.";
	minTargets = 1;
	addRequirementsDescription();
}
void SelfCombust::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	triggerEvents(std::vector<CombatProfile*> ());
}
void SelfCombust::triggerEvents(std::vector<CombatProfile*> profiles) {
	Spell::triggerEvents(profiles);
	source->HP = 0;
	ENCOUNTER->updateHistory <TriggerChecker, Event> (nullptr);
	for (auto it : validTargets) {
		DamageEvent* dmg = new FixedDamageEvent(100);
		dmg->src = source;
		dmg->dest = it->cp;
		ENCOUNTER->addEventOnStack(dmg);
	}
}

Assault::Assault() {
	name = "Assault";
	setTilesOffset(4, rangeTilesOffset);
	setTilesOffset(0, effectTilesOffset);
	addRequirements(new Cooldown(3, 3), new APCostRequirement(3), new MPCostRequirement(2));
	fantasy = " Assault\n\n    Why do beings tend so easily towards violence? Why do we keep hurting each other? Maybe that's just because it is fun.\n";
	addDescription("Range", "4 tiles around caster");
	techDescription = "    Target an enemy with available adjacent tiles - teleport to a random one and deal 1-hit 150% damage.";
	addRequirementsDescription();
}
void Assault::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	ENCOUNTER->combatMoveTargetRequestSetup(this, 1);
}
void Assault::triggerEvents(std::vector<CombatProfile*> profiles) {
	CombatProfile* target = profiles[0];
	std::pair <int, int> p(-1, -1);
	std::pair <int, int> dxy[] = { {-1, 0}, {1, 0}, {0, 1}, {0, -1} };
	random_shuffle(dxy, dxy + 4);
	for (int i = 0; i < 4; ++i) {
		std::pair <int, int> x = { dxy[i].first + target->grid_x, dxy[i].second + target->grid_y };
		if (ENCOUNTER->checkValability(x)) p = x, i = 5;
	}

	if (p.first == -1) {
		ENCOUNTER->addErrorMessage("Cannot teleport, no available adjacent tiles!");
	}
	else {
		Spell::triggerEvents(profiles);
		TeleportEvent* event = new TeleportEvent(p);
		event->src = source;

		source->grid_x = p.first;
		source->grid_y = p.second;
		source->attackTarget = target;
		source->ch->requestAnimation("attack", "idle");

		DamageEvent* dmg = new FixedDamageEvent(150);
		dmg->src = source;
		dmg->dest = target;

		ENCOUNTER->addEventOnStack(dmg);
		ENCOUNTER->addEventOnStack(event);
	}
}

Hate::Hate() {
	name = "Hate";
	setTilesOffset(1, rangeTilesOffset);
	setTilesOffset(0, effectTilesOffset);
	addRequirements(new Cooldown(4, 4), new APCostRequirement(3));
	fantasy = " Hate\n\n    In the winter of one's emotions, only hate is able to bring warmth.\n";
	addDescription("Range", "melee");
	techDescription = "    Attack an enemy and deal 100% - 400% damage, depending on how much HP you're missing.";
	addRequirementsDescription();
}
void Hate::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	ENCOUNTER->combatMoveTargetRequestSetup(this, 1);
}
void Hate::triggerEvents(std::vector<CombatProfile*> profiles) {
	Spell::triggerEvents(profiles);
	CombatProfile* target = profiles[0];
	FixedDamageEvent* damageEvent;
	float scale = (float)source->HP / source->init->HP;
	int value = (int) (100 + 300 * (1 - scale));
	damageEvent = new FixedDamageEvent(value);
	damageEvent->src = source;
	damageEvent->dest = target;
	source->attackTarget = target;
	source->ch->requestAnimation("attack", "idle");
	ENCOUNTER->addEventOnStack(damageEvent);
}
Backstab::Backstab() {
	name = "Backstab";
	setTilesOffset(1, rangeTilesOffset);
	setTilesOffset(0, effectTilesOffset);
	addRequirements(new Cooldown(2, 2), new APCostRequirement(3));
	fantasy = " Hate\n\n    Nice weakpoint you have there - it would be a shame not to put it to use!\n";
	addDescription("Range", "melee");
	addDescription("VULNERABILITY tile", vulnerable_description);
	techDescription = "    Attack an enemy and deal 1-hit 150% damage. If you're standing on the vulnerability tile of an enemy, consume it and deal 400% damage.";
	addRequirementsDescription();
}
void Backstab::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	ENCOUNTER->combatMoveTargetRequestSetup(this, 1);
}
void Backstab::triggerEvents(std::vector<CombatProfile*> profiles) {
	Spell::triggerEvents(profiles);
	CombatProfile* target = profiles[0];
	FixedDamageEvent* damageEvent;
	if (profiles[0]->getVulnerableTile() == source->getTile()) {
		damageEvent = new FixedDamageEvent(400);
		damageEvent->bonus = "BACKSTAB! ";
		VulnerabilityConsumptionEvent* other = new VulnerabilityConsumptionEvent();
		other->dest = profiles[0];
		ENCOUNTER->addEventOnStack(other);
	}
	else {
		damageEvent = new FixedDamageEvent(150);
	}
	damageEvent->src = source;
	damageEvent->dest = target;
	source->attackTarget = target;
	source->ch->requestAnimation("attack", "idle");
	ENCOUNTER->addEventOnStack(damageEvent);
}

LifeGamble::LifeGamble() {
	name = "Life Gamble";
	setTilesOffset(1, rangeTilesOffset);
	setTilesOffset(0, effectTilesOffset);
	addRequirements(new Cooldown(3, 3), new APCostRequirement(3));
	fantasy = " Life Gamble\n\n    Place your bets, everyone - will this bring them down, or will it not?\n";
	addDescription("Range", "melee");
	addDescription("VULNERABILITY tile", vulnerable_description);
	addDescription("On Kill Rewards", "6AP, 4MP, cooldowns reset");
	techDescription = "    Attack an enemy and deal 1-hit 100% damage. Deal 200% damage standing on the vulnerability tile of an enemy. If this attack kills your target, increase your AP by 6, your MP by 4, and reset ALL your cooldowns.";
	addRequirementsDescription();
}
void LifeGamble::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	ENCOUNTER->combatMoveTargetRequestSetup(this, 1);
}
void LifeGamble::triggerEvents(std::vector<CombatProfile*> profiles) {
	Spell::triggerEvents(profiles);
	CombatProfile* target = profiles[0];
	FixedDamageEvent* damageEvent;

	if (!ENCOUNTER->bSimMode) new KillRewardTrigger(profiles[0], source);
	
	if (profiles[0]->getVulnerableTile() == source->getTile()) {
		damageEvent = new FixedDamageEvent(200);
		damageEvent->bonus = "BACKSTAB! ";
		VulnerabilityConsumptionEvent* other = new VulnerabilityConsumptionEvent();
		other->dest = profiles[0];
		ENCOUNTER->addEventOnStack(other);
	}
	else {
		damageEvent = new FixedDamageEvent(100);
	}
	damageEvent->src = source;
	damageEvent->dest = target;
	source->attackTarget = target;
	source->ch->requestAnimation("attack", "idle");
	ENCOUNTER->addEventOnStack(damageEvent);
}

HealingFountain::HealingFountain(int value) {
	color = DCYAN;
	this->value = value;
	name = "Heal Fountain";
	addRequirements(new APCostRequirement(3), new Cooldown(5, 5));
	setTilesOffset(1, effectTilesOffset);
	setTilesOffset(5, rangeTilesOffset);
	fantasy = " Healing Fountain\n\n    Create for all that you have destroyed.\n";
	techDescription = "    Heal an ally target and all adjacent allies for " + intToStr(value) + "% of their max HP.";
	addRequirementsDescription();
}
void HealingFountain::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	ENCOUNTER->combatMoveTargetRequestSetup(this, 1);
}
void HealingFountain::triggerEvents(std::vector<CombatProfile*> profiles) {
	Spell::triggerEvents(profiles);
	for (auto it : Encounter::getInstance()->getProfiles()) {
		int dist = abs(it->grid_x - profiles[0]->grid_x) + abs(it->grid_y - profiles[0]->grid_y);
		if (dist == 1 && it->sentience == source->sentience) profiles.push_back(it);
	}

	for (auto it : profiles) {
		HealEvent* event = new HealEvent(value);
		event->src = source;
		event->dest = it;
		source->ch->requestAnimation("heal", "idle");
		ENCOUNTER->addEventOnStack(event);
	}
}

//	SPELLS OVER

//	ATTACKS

DaggerAttack::DaggerAttack() {
	name = "Dagger Attack";
	addRequirements(new APCostRequirement(3));
	setTilesOffset(1, rangeTilesOffset);
	fantasy = " Slash\n\n    Sometimes, the most simple strategies are the most effective.\n";
	techDescription = "    Deal 1-hit 100% damage to your target. ";
	addRequirementsDescription();
}	
void DaggerAttack::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	std::vector<CombatProfile*> profiles;
	profiles.push_back(source->attackTarget);
	triggerEvents(profiles);
}
void DaggerAttack::triggerEvents(std::vector<CombatProfile*> profiles) {
	Attack::triggerEvents(profiles);
	FixedDamageEvent* damageEvent;
	damageEvent = new FixedDamageEvent(100);
	damageEvent->src = source;
	damageEvent->dest = profiles[0];
	ENCOUNTER->addEventOnStack(damageEvent);
}

Claw::Claw() {
	name = "Claw";
	addRequirements(new APCostRequirement(2));
	setTilesOffset(1, rangeTilesOffset);
	fantasy = " Claw\n\n    Devils know to hit where it hurts most!\n";
	techDescription = "    Deal 1-hit 100% damage to your target. ";
	addRequirementsDescription();
}	
void Claw::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	std::vector<CombatProfile*> profiles;
	profiles.push_back(source->attackTarget);
	triggerEvents(profiles);
}
void Claw::triggerEvents(std::vector<CombatProfile*> profiles) {
	Attack::triggerEvents(profiles);
	FixedDamageEvent* damageEvent;
	damageEvent = new FixedDamageEvent(100);
	damageEvent->src = source;
	damageEvent->dest = profiles[0];
	ENCOUNTER->addEventOnStack(damageEvent);
}

ArrowAttack::ArrowAttack() {
	name = "Arrow Attack";
	addRequirements(new APCostRequirement(3));
	setLineTilesOffset(8, rangeTilesOffset);
	fantasy = " Arrow Attack\n\n    Apple on your head or not, you're still great aiming practice!\n";
	techDescription = "    Deal 1-hit 100% damage to your target in vertical range.";
	addRequirementsDescription();
}
void ArrowAttack::castMove(CombatProfile* profile) {
	CombatMoveInterface::castMove(profile);
	std::vector<CombatProfile*> profiles;
	profiles.push_back(source->attackTarget);
	triggerEvents(profiles);
}
void ArrowAttack::triggerEvents(std::vector<CombatProfile*> profiles) {
	Attack::triggerEvents(profiles);
	ScreenObject* obj = new ScreenObject(source->ch->getPosition().first+8, source->ch->getPosition().second-2, 50, "arrow");
	obj->show();
	if (profiles[0]->grid_x < source->grid_x) obj->flipX(4);
	new ObjectPositionXTransform <ScreenObject>(obj, 10.0f, 10.0f, profiles[0]->ch->getPosition().first, true);

	FixedDamageEvent* damageEvent;
	damageEvent = new FixedDamageEvent(100);
	damageEvent->src = source;
	damageEvent->dest = profiles[0];
	ENCOUNTER->addEventOnStack(damageEvent);
}


//	ATTACKS OVER

//	GENERICS

void Spell::triggerEvents(std::vector<CombatProfile*> profiles) {
	source->lastMove = this;
	if (ENCOUNTER->bSimMode) return;
	for (auto it : requirements) it->applyConsequences(source);
	Event* spellcastEvent = new Event(); spellcastEvent->src = source;
	source->lastSpellcast = this;
	ENCOUNTER->updateHistory <SpellcastLog> (spellcastEvent);
};
void Attack::triggerEvents(std::vector<CombatProfile*> profiles) {
	source->lastMove = this;
	if (ENCOUNTER->bSimMode) return;
	for (auto it : requirements) it->applyConsequences(source);
	Event* attackcastEvent = new Event(); attackcastEvent->src = source;
	source->lastAttackcast = this;
	ENCOUNTER->updateHistory <AttackcastLog> (attackcastEvent);
}

//	AI

PrimitiveAIBrain::PrimitiveAIBrain(Character* ch) : ch(ch) {
	target = nullptr;
	spell = nullptr;
	randIdx = 0;
	didNothingThisTurn = false;
	defaultLastResortResponse = true;
}
PrimitiveAIBrain::~PrimitiveAIBrain() {}

void PrimitiveAIBrain::resetData() {
	tileAI = { -1, -1 };
	spell = nullptr;
	target = nullptr;
}

void PrimitiveAIBrain::runCombatAI() {
	if (ch->cp->bDead) {
		while (!actionQueue.empty())
			actionQueue.pop();
		actionQueue.push(-1);
		return;
	}
	
	recomputeRandArr();
	resetData();

	attackCheck();
	for (auto& sp : ch->cp->spellArr)
		spellCheck(sp);

	std::vector <std::pair <float, int>> arr;
	for (int i = 0; i < (int)moves.size(); ++i) {
		if (moves[i]->score > 0)
			arr.push_back({ moves[i]->score, i });
		//	DEBUG_PRINT(intToStr(moves[i]->score), 5.0f);
	}

	if (arr.empty()) {
		if (didNothingThisTurn)
			doLastResortMovement();
		else actionQueue.push(-1);
	}
	else {
		didNothingThisTurn = false;
		moves[randomArrElementWeighted(arr)]->call();
	}

	for (auto it : moves) delete it;
	moves.clear();
}

void PrimitiveAIBrain::attackCheck() {
	if (ch->cp->attackArr.empty()) return;
	Attack* atk = ch->cp->attackArr[0];
	CombatProfile* cp = ch->cp;

	if (cp->MP == 0 || !atk->checkRequirements(cp)) return;

	std::vector <Character*> chList = ENCOUNTER->getCharacters();
	std::vector <std::tuple <CombatProfile*, int, std::pair <int, int>>> arr;

	for (auto targetCandidate : ENCOUNTER->getProfiles()) {
		if (!isEnemy(targetCandidate)) continue;
		auto map = LAYOUTS->AIHelper(GRID, targetCandidate->grid_x, targetCandidate->grid_y, cp->grid_x, cp->grid_y);
		auto forbidden = LAYOUTS->getForbiddenDestinationTiles(GRID);

		auto tile = chooseAttackTile(map, forbidden);
		
		if (abs(targetCandidate->grid_x - tile.first) + abs(targetCandidate->grid_y - tile.second) <= cp->attackRange)
			arr.push_back(std::make_tuple(targetCandidate, map[tile].second, tile));
	};

	if (arr.empty()) return;

	CombatProfile* targetCandidate = nullptr;
	std::pair <int, int> tile;
	int cost;

	std::vector <std::pair <float, int>> weightedArr;
	for (int i = 0; i < (int) arr.size(); ++i) {
		std::tie(targetCandidate, cost, tile) = arr[i];
		float score = computeTargetCandidateScore(targetCandidate, cost, tile);
		weightedArr.push_back({ score, i });
	}

	std::tie(targetCandidate, cost, tile) = arr[randomArrElementWeighted(weightedArr)];

	float score = computeTargetCandidateScore(targetCandidate, cost, tile);
	moves.push_back(new AIAttack(targetCandidate, this, score, tile));
}

bool PrimitiveAIBrain::spellCheck(Spell* candidate) {
	CombatProfile* cp = ch->cp;

	auto map = LAYOUTS->calcDist(GRID, cp->grid_x, cp->grid_y);
	auto forbidden = LAYOUTS->getForbiddenDestinationTiles(GRID);
	forbidden.erase(cp->getTile());

	std::vector <std::pair <std::pair <int, int>, int>> tiles;
	for (auto& it : map) {
		if (!ENCOUNTER->checkValability(it.first) && it.first != cp->getTile()) continue;
		if (forbidden.find(it.first) == forbidden.end() && it.second <= cp->MP) {
			int x = it.first.first;
			int y = it.first.second;
			int cpy_x = cp->grid_x;
			int cpy_y = cp->grid_y;
			int mpCost = it.second;
			int v = it.second;

			cp->grid_x = x;
			cp->grid_y = y;
			cp->MP -= mpCost;
			if (candidate->checkMoveValidity(ch, ENCOUNTER->getCharacters()) == VC_OK
				&& candidate->checkRequirements(cp)) {
				if (tiles.empty()) {
					tiles.push_back(it);
				}
				else {
					if (areEqual(it, tiles.back(), candidate))
						tiles.push_back(it);
					else if (isBetter(it, tiles.back(), candidate)) {
						tiles.clear();
						tiles.push_back(it);
					}
				}
			}

			cp->grid_x = cpy_x;
			cp->grid_y = cpy_y;
			cp->MP += mpCost;
		}
	}

	if (tiles.empty()) return false;

	tileAI = tiles[Rand() % tiles.size()].first;
	moves.push_back(new AISpell(candidate, this, computeScore(candidate, tileAI, map[tileAI]), tileAI));

	return true;
}

std::pair<int, int> PrimitiveAIBrain::chooseAttackTile(std::map<std::pair<int, int>, std::pair<int, int>> map, std::set<std::pair<int, int>> forbidden) {
	std::pair <std::pair <int, int>, std::pair <int, int>> ans = { {-1, -1}, {(int)2e9, (int)2e9} };

	CombatProfile* cp = ch->cp;

	forbidden.erase(cp->getTile());
	std::vector <std::pair <int, int>> poss;
	poss.push_back(ans.first);
	for (auto& it : map) {
		if (!ENCOUNTER->checkValability(it.first) && it.first != cp->getTile()) continue;
		if (forbidden.find(it.first) == forbidden.end() && it.second.second <= cp->MP) {
			int x = it.first.first;
			int y = it.first.second;
			int cpy_x = cp->grid_x;
			int cpy_y = cp->grid_y;
			int mpCost = it.second.second;

			auto cpy = cp->getTile();

			cp->grid_x = x;
			cp->grid_y = y;
			cp->MP -= mpCost;

			if (cp->attackArr[0]->checkMoveValidity(ch, ENCOUNTER->getCharacters()) == VC_OK && cp->attackArr[0]->checkRequirements(ch->cp)) {
				if (ans.second > it.second)
					poss.clear(), poss.push_back(it.first), ans = it;
				else if (ans.second == it.second) {
					poss.push_back(it.first);
				}
			}

			cp->grid_x = cpy_x;
			cp->grid_y = cpy_y;
			cp->MP += mpCost;
		}
	}

	if (poss.empty()) {
		return std::pair <int, int>(-1, -1);
	}
	return randomArrElement(poss);
}


float PrimitiveAIBrain::computeTargetCandidateScore(CombatProfile* target, int mpCost, std::pair<int, int> tile) {
	float score = (float) (100 - 1.0 * target->HP / target->init->HP * 100.0);
	return score;
}
float PrimitiveAIBrain::computeScore(Spell* spell, std::pair<int, int> tile, int cost) {
	ENCOUNTER->turnOnSim();
	CombatProfile* cp = ch->cp;
	int x = tile.first;
	int y = tile.second;
	int cpy_x = cp->grid_x;
	int cpy_y = cp->grid_y;
	cp->grid_x = x;
	cp->grid_y = y;
	cp->MP -= cost;

	spell->castMove(cp);
	spell->triggerEvents(targetArr);

	auto arr = ENCOUNTER->simArr;
	float score = parseEventArray(arr);

	ENCOUNTER->turnOffSim();
	cp->grid_x = cpy_x;
	cp->grid_y = cpy_y;
	cp->MP += cost;

	return score;
}

float PrimitiveAIBrain::parseEventArray(std::vector <Event*> arr) {
	float score = 0;
	for (auto it : arr) {
		FixedDamageEvent* fde = dynamic_cast <FixedDamageEvent*> (it);
		if (fde != nullptr) {
			score += fde->getValue();
			continue;
		}
		TrueScallingMAXHPDamageEvent* scallingde = dynamic_cast <TrueScallingMAXHPDamageEvent*> (it);
		if (scallingde != nullptr) {
			score += scallingde->getValue();
			continue;
		}
	}
	return score;
}

void PrimitiveAIBrain::attackSetup(CombatProfile* targetCandidate, std::pair <int, int> tile) {
	tileAI = tile;
	target = targetCandidate;
	actionQueue.push(0);
	actionQueue.push(1);
}
void PrimitiveAIBrain::spellSetup(Spell* spellCandidate, std::pair <int, int> tile) {
	tileAI = tile;
	actionQueue.push(0);
	actionQueue.push(2);
	spell = spellCandidate;
}

bool PrimitiveAIBrain::isEnemy(CombatProfile* profile) {
	return profile->sentience == PLAYER;
}
bool PrimitiveAIBrain::areEqual(std::pair<std::pair<int, int>, int> candidate, std::pair<std::pair<int, int>, int> old, Spell *spell) {
	return candidate.second == old.second;
}
bool PrimitiveAIBrain::isBetter(std::pair<std::pair<int, int>, int> candidate, std::pair<std::pair<int, int>, int> old, Spell* spell) {
	return candidate.second < old.second;
}

void PrimitiveAIBrain::solveSelector(InputSingleSelector* selector) {
	int v = getRand() % selector->choices.size();
	selector->choice = selector->choices[v];
}

void PrimitiveAIBrain::targetSetup(int count, bool bDistinct, Spell *spell) {
	resetRand();
	targetArr.clear();

	spell->checkMoveValidity(ch, ENCOUNTER->getCharacters());
	auto arrCopy = spell->getValidTargets();

	for (int i = 0; i < count; ++i) {
		if (arrCopy.empty()) return;
		int v = getRand() % arrCopy.size();
		targetArr.push_back(arrCopy[v]->cp);

		if (bDistinct) arrCopy.erase(arrCopy.begin() + v);
	}
}

bool PrimitiveAIBrain::makeLastResortDecision() {
	return defaultLastResortResponse;
}
void PrimitiveAIBrain::doLastResortMovement() {
	didNothingThisTurn = false;
	
	std::pair <int, int> tile(-1, -1);
	if (makeLastResortDecision())
		tile = chooseAdvancementTile();
	else
		tile = chooseRetreatTile();

	if (tile.first != -1) {
		actionQueue.push(0);
		tileAI = tile;
	}
	else {
		actionQueue.push(-1);
	}
}

std::pair<int, int> PrimitiveAIBrain::chooseAdvancementTile() {
	std::pair<int, int> ans(-1, -1);
	CombatProfile* cp = ch->cp;
	CombatProfile* target = nullptr;

	auto aux = LAYOUTS->calcDist(GRID, cp->grid_x, cp->grid_y);

	int dist = (int) 2e9;
	for (auto& it : ENCOUNTER->getProfiles()) {
		if (isEnemy(it) && dist > aux[it->getTile()]) {
			target = it;
			dist = aux[it->getTile()];
		}
	}

	auto map = LAYOUTS->AIHelper(GRID, target->grid_x, target->grid_y, cp->grid_x, cp->grid_y);
	auto forbidden = LAYOUTS->getForbiddenDestinationTiles(GRID);
	forbidden.erase(cp->getTile());

	int maxMP = cp->MP;
	int min = (int) 2e9;

	for (auto it : map) {
		if (!ENCOUNTER->checkValability(it.first) && it.first != cp->getTile()) continue;
		if (forbidden.find(it.first) != forbidden.end()) continue;
		if (it.second.second <= maxMP) {
			if (it.second.first < min) {
				min = it.second.first;
				ans = it.first;
			}
			else if (it.second.first == min) {
				int v1 = abs(cp->grid_x - ans.first) * abs(cp->grid_y - ans.second);
				int v2 = abs(cp->grid_x - it.first.first) * abs(cp->grid_y - it.first.second);
				if (v2 > v1) ans = it.first;
			}
		}
	}

	if (ans == cp->getTile()) ans = { -1, -1 };
	return ans;	
}
std::pair<int, int> PrimitiveAIBrain::chooseRetreatTile() {
	std::pair<int, int> ans(-1, -1);
	CombatProfile* cp = ch->cp;
	CombatProfile* target = nullptr;

	auto aux = LAYOUTS->calcDist(GRID, cp->grid_x, cp->grid_y);

	int dist = (int) 2e9;
	for (auto& it : ENCOUNTER->getProfiles()) {
		if (isEnemy(it) && dist > aux[it->getTile()]) {
			target = it;
			dist = aux[it->getTile()];
		}
	}

	if (target == nullptr) return ans;
	auto map = LAYOUTS->AIHelper(GRID, target->grid_x, target->grid_y, cp->grid_x, cp->grid_y);
	auto forbidden = LAYOUTS->getForbiddenDestinationTiles(GRID);
	forbidden.erase(cp->getTile());

	int maxMP = cp->MP;
	int max = (int) -2e9;

	for (auto it : map) {
		if (!ENCOUNTER->checkValability(it.first) && it.first != cp->getTile()) continue;
		if (forbidden.find(it.first) != forbidden.end()) continue;
		if (it.second.second <= maxMP) {
			if (it.second.first > max) {
				max = it.second.first;
				ans = it.first;
			}
			else if (it.second.first == max) {
				int v1 = abs(target->grid_x - ans.first) * abs(target->grid_y - ans.second);
				int v2 = abs(target->grid_x - it.first.first) * abs(target->grid_y - it.first.second);
				if (v2 < v1) ans = it.first;
			}
		}
	}

	if (ans == cp->getTile()) ans = { -1, -1 };
	return ans;
}

void PrimitiveAIBrain::actionZero() {
	CombatProfile* cp = ch->cp;

	if (tileAI != std::pair <int, int> (-1, -1)) {
		if (tileAI == cp->getTile()) return;
		ENCOUNTER->pressMovement();
		ENCOUNTER->pressTile(tileAI);
		return;
	}
	if (target == nullptr) return;
}
void PrimitiveAIBrain::actionOne() {
	CombatProfile* cp = ch->cp;

	Attack* atk = cp->attackArr[0];
	if (atk->checkRequirements(cp)) {
		int dist = abs(cp->grid_x - target->grid_x) + abs(cp->grid_y - target->grid_y);
		if (dist <= cp->attackRange) {
			ENCOUNTER->pressAttack(target);

			if (atk->checkRequirements(cp)) {
				actionQueue.push(1);
			}
		}
	}
}
void PrimitiveAIBrain::actionTwo() {
	resetRand();
	ENCOUNTER->pressSkills();
}
void PrimitiveAIBrain::actionMinus() {
	ENCOUNTER->pressEndTurn();
}

AIAttack::AIAttack(CombatProfile* target, PrimitiveAIBrain* brain, float score, std::pair <int, int> tile) : target(target), tile(tile), AIMove(brain, score) {}
AISpell::AISpell(Spell *spell, PrimitiveAIBrain* brain, float score, std::pair <int, int> tile) : spell(spell), tile(tile), AIMove(brain, score) {}

void AIAttack::call() {
	brain->attackSetup(target, tile);
}
void AISpell::call() {
	brain->spellSetup(spell, tile);
}

ImpBrain::ImpBrain(Character* ch) : PrimitiveAIBrain(ch) {}

float ImpBrain::computeTargetCandidateScore(CombatProfile* target, int mpCost, std::pair<int, int> tile) {
	float score = 1.0f * 1000 / target->init->HP;
	return score;
}

float ImpBrain::computeScore(Spell* spell, std::pair<int, int> tile, int cost) {
	if (spell->getName() == "Self Combust") {
		int v = getAlliesAroundTile(tile); 
		if (v == 0) return 10000.0f;
		else return 0.0f;
	}
	else if (spell->getName() == "Assault") {
		return 0.0f;
	}
	
	return PrimitiveAIBrain::computeScore(spell, tile, cost);
}

int ImpBrain::getAlliesAroundTile(std::pair<int, int> tile) {
	int counter = 0;
	for (auto it : ENCOUNTER->getCharacters()) {
		if (it->cp->sentience == AI && it != ch && !it->cp->bDead) {
			int dist = abs(it->cp->grid_x - tile.first) + abs(it->cp->grid_y - tile.second);
			if (dist <= 6) ++ counter;
		}
	}	return counter;
}

void ImpBrain::doLastResortMovement() {
	for (auto& it : ch->cp->spellArr) {
		if (it->getName() == "Assault")
			spellCheck(it);
	}

	if (!moves.empty()) {
		moves[0]->call();
	}
	
	PrimitiveAIBrain::doLastResortMovement();
}

bool ImpBrain::areEqual(std::pair<std::pair<int, int>, int> candidate, std::pair<std::pair<int, int>, int> old, Spell* spell) {
	if (spell == nullptr) return false;
	if (spell->getName() == "Self Combust") {
		int v1 = getAlliesAroundTile(old.first);
		int v2 = getAlliesAroundTile(candidate.first);
		if (v1 == v2) return PrimitiveAIBrain::areEqual(candidate, old, spell);
		else return false;
	}
	
	return PrimitiveAIBrain::areEqual(candidate, old, spell);
}

bool ImpBrain::isBetter(std::pair<std::pair<int, int>, int> candidate, std::pair<std::pair<int, int>, int> old, Spell* spell) {
	if (spell == nullptr) return false;
	if (spell->getName() == "Self Combust") {
		int v1 = getAlliesAroundTile(old.first);
		int v2 = getAlliesAroundTile(candidate.first);
		if (v1 == v2) return PrimitiveAIBrain::isBetter(candidate, old, spell);
		else return v1 > v2;
	}

	return PrimitiveAIBrain::isBetter(candidate, old, spell);
}
