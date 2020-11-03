#include "VisualObject.h"

#include "consolewizard.h"

#define BUFFER_SIZE 1005

std::map <char, char> VisualObject::flipRules = { {'>', '<'}, {'\\', '/'}, {'/', '\\'}, {'(', ')'}, {')', '('}, {'[', ']'}, {']', '['}, {'{', '}'}, {'}', '{'} };

VisualObject::VisualObject() {
	x = y = renderPlane = 0;
	bOverrideInnateRules = 0;
	bHide = 0;
	setName("Visual");
}

VisualObject::~VisualObject() {
}

void VisualObject::destroy() {
	PrimitiveObject::destroy();
}

inline int VisualObject::render_order() { return x + y * getwidth(); }

void VisualObject::loop(float delta) {
	if (bDestroyed) return;
	PrimitiveObject::loop(delta);
}

std::vector <OutputData> VisualObject::readOutputDataFromFile(std::ifstream& file) {
	std::vector <OutputData> ret;
	bool done = false;
	char str[BUFFER_SIZE];
	int x, y = 0;
	while (!done) {
		file.getline(str, BUFFER_SIZE);
		if (str[0] == '#') { done = true; continue; }

		x = 0;
		int idx = 0;
		int len = strlen(str);
		while (idx < len) {
			while (str[idx] == ' ' && idx < len) ++idx, ++x;
			if (idx < len) {
				ret.push_back({ y, x, "" });
				while (str[idx] != ' ' && idx < len) {
					if (str[idx] == '5')
						ret.back().str += " ", ++idx, ++x;
					else
						ret.back().str += str[idx++], ++x;
				}
			}
		}   ++y;
	}   return ret;
}

void VisualObject::flipX(std::vector<OutputData>& data, int xAxis) {
	for (auto& it : data) {
		std::reverse(it.str.begin(), it.str.end());
		for (auto& ch : it.str) {
			if (flipRules.find(ch) != flipRules.end())
				ch = flipRules[ch];
		}
		it.x = xAxis - (it.x + it.len());
	}
}
