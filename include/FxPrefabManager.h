#pragma once

#include <archimedes/Scene.h>
#include <box2d/box2d.h>
#include <nlohmann/json.hpp>

using namespace arch;

using Json = nlohmann::json;

struct HealthData {
	float value;
	bool relative;
};

struct PolygonData {
	b2Polygon x;
};

struct FxPrefab {
	b2BodyDef bodyDef;
	b2ShapeDef shapeDef;
	b2ShapeType shapeType;
	union {
		b2Circle circle;
		b2Polygon polygon;
	} shapeData;
	std::optional<HealthData> healthDataOpt;
	Ref<gfx::pipeline::Pipeline> texture;
	std::string name;

	Json json;
};

struct FxPrefabId {
	std::string value;
};

struct FxPrefabManager {
	static OptRef<const FxPrefab> get(std::string_view name);
	static void clear();
};