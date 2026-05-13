#pragma once

enum class LevelState {
	playing,
	lost,
	won
};

struct EndingSystem {
	static void update();
};