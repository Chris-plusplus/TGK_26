#pragma once

#include <archimedes/Scene.h>
#include <Cell.h>

using namespace arch;

struct Grid {
	std::vector<Cell*> cells;
	float2 bottomLeft;
	float2 topRight;
	u32 size;
	u32 i;
};