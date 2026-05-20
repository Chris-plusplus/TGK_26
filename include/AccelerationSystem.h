#pragma once
#include <archimedes/Gfx.h>

using namespace arch;

struct Acceleration {
	float value;
	Ref<gfx::pipeline::Pipeline> textureOnHit;
};

struct AccelerationSystem {
	static void update();
};