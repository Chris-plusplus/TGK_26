#pragma once

struct Collided {
	float timeSince;
};

struct DamageToOthers {
	float multiplier = 1;
};

struct CollisionSystem {
	static void update();
};