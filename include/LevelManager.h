#pragma once

#include <string>
#include <optional>
#include <archimedes/ArchMath.h>

using namespace arch;

struct LevelEntity {
	static constexpr bool flagComponent = true;
};

struct LevelData {
	std::string current;
	std::string next;

	float cameraMinZoom;
	float cameraMaxZoom;
	float2 cameraMinPosition;
	float2 cameraMaxPosition;
	float zoomFactor;
};

struct LevelManager {
	static void loadLevel(std::string_view levelName);
	static void reloadLevel();
	static void nextLevel();
	static void clearLevel();
};