#pragma once

#include <archimedes/Scene.h>
#include <Grid.h>
#include <Cell.h>

using namespace arch;

struct CellMember {
	Grid* grid = nullptr;
	Cell* cell = nullptr;
};