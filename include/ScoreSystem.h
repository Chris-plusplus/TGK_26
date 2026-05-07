#pragma once

struct ScoreData {
	float value = 0;
	bool update = true;
};

struct ScoreSystem {
	static void add(float value);
	static void reset();
	static float get();
	static void setup();
	static void update();
};