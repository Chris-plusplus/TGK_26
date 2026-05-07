#pragma once

struct Collided {
	static constexpr bool flagComponent = true;
};

struct CollisionSystem {
	static void update();
};