#pragma once

#include <archimedes/Scene.h>
#include <CellMember.h>

using namespace arch;

struct CellSystem {
	static void init();
	static void initCities();
	static void initPopulation();
	static void rebuild();
	static std::vector<ecs::Entity> findNeighbors(ecs::Entity actor);
};