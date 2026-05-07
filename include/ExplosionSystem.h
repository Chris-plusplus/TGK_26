#pragma once

#include <archimedes/Scene.h>
#include <box2d/box2d.h>

using namespace arch;

struct Explosion {
	b2ExplosionDef explosionDef;
	float damageModifier;
	float timer;
};

struct ExplosionSystem {
	static void update();
};