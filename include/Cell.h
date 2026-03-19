#pragma once

#include <archimedes/Scene.h>
#include <vector>

using namespace arch;

struct Cell {
	std::vector<std::pair<ecs::Entity, float2>> actors;
	std::vector<Cell*> neighbors;
};