#pragma once

#include <nlohmann/json.hpp>
#include <archimedes/Scene.h>

using namespace arch;

struct SimConfig {
	struct City {
		f32 bottom;
		f32 left;
		f32 length;
		NLOHMANN_DEFINE_TYPE_INTRUSIVE(City, bottom, left, length);
	};
	std::vector<City> cities;

	f32 maxAngleJerk;
	f32 maxAngleChange;
	f32 actorSpeed;

	f32 infectionRadius;
	f32 infectionChance;
	u32 initialInfected;
	u32 populationPerCity;
	u32 timeToRemove;

	NLOHMANN_DEFINE_TYPE_INTRUSIVE(SimConfig,
		cities,

		maxAngleJerk,
		maxAngleChange,
		actorSpeed,

		infectionRadius,
		infectionChance,
		initialInfected,
		populationPerCity,
		timeToRemove
	);
};

extern SimConfig simConfig;