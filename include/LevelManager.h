#pragma once

#include <string>
#include <optional>
#include <archimedes/ArchMath.h>

struct LevelEntity {
	static constexpr bool flagComponent = true;
};

struct LevelManager {
	static void loadLevel(std::string_view levelName);
	static void reloadLevel();
	static void nextLevel();
	static void clearLevel();
};