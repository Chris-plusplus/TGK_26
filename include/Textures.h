#pragma once

#include <archimedes/gfx/pipeline/Pipeline.h>

using namespace arch;

struct Textures {
	Ref<gfx::pipeline::Pipeline> susceptible;
	Ref<gfx::pipeline::Pipeline> infected;
	Ref<gfx::pipeline::Pipeline> removed;
};