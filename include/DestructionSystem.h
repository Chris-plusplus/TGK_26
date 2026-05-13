#pragma once

#include <archimedes/Scene.h>

using namespace arch;

struct Destructible {
	float health = 0;
	float destructionPoints = 0;
	float damage = 0;
};

struct DestructibleData {
	float health = 0;
	float destructionPoints = 0;
	bool relativeHealth = false;
};

struct DestructionSystem {
	static void update();
};