#include "engine/Debugger.h"

#include "engine/GodClass.h"

#include "engine/stdc++.h"

Debugger* debug;
void mainInitialize() {
	ResetRand();
	GOD;	// initializes GodClass
	std::cin.tie(NULL);
	std::cout.tie(NULL);
	srand((unsigned int)time(NULL));

	debug = new Debugger;
	if (DEBUG_MODE) debug->initialize(), GOD->setDebugger(debug);
}

int main()
{
	mainInitialize();

	std::clock_t present, time;
	float seconds;

	time = clock();

	float total = 0;
	while (true) {
		present = clock(); seconds = ((present - time) / (float)CLOCKS_PER_SEC);
		if (seconds < 0.01f) continue;
		total += seconds;
		GOD->loop(seconds);
		if (debug) debug->loop(seconds);
		time = present;
		if (total > 200000000.0f) {
			delete GOD;
			break;
		}
	}   if (debug) delete debug;

	return 0;
}
