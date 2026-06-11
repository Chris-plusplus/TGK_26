#pragma once

#include <FxPrefabManager.h>

struct Slingshot {
	float canReloadTimePassed = 0;
	float canReloadTime = 0;
	float3 centerPos = {};
	float maxPull = 100;
	float forceMultiplier = 1;

	enum State {
		waiting,
		reloading,
		loaded,
		empty,
		dragged
	} state = waiting;
};

struct Can {
	const FxPrefab* prefab;
	float points = 0;
};

struct CanOnSlingshot {
	float progress = 0;
	float deltaProgressPerSec = 1.f;
	float3 posBegin = {};
};

struct Launched {
	static constexpr bool flagComponent = true;
};

struct Band {
	static constexpr bool flagComponent = true;
};

struct SlingshotSystem {
	static Entity placeSlingshot(float2 pos, std::string_view texturePath);
	static void moveCanStep(float dt);

	static void update();
};