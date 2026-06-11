#pragma once
#include <archimedes/Gfx.h>
#include <random>

using namespace arch;

struct Acceleration {
	Ref<gfx::pipeline::Pipeline> textureOnUse;
	Ref<gfx::texture::Texture> foamTexture;
	Ref<gfx::pipeline::Pipeline> foamPipeline;
	std::uniform_real_distribution<float> foamAngleDist;
	std::uniform_real_distribution<float> foamRotationDist;
	std::uniform_real_distribution<float> foamVelocityDist;
	std::minstd_rand rng;
	float value;
	float foamTime;
	u32 newFoams;
	float foamSpeedLoss;
};

struct AccelerationSystem {
	static void update();
};