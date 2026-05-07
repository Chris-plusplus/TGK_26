#include <FxPrefabManager.h>
#include <nlohmann/json.hpp>
#include <Defaults.h>
#include <Utils.h>

using Json = nlohmann::json;

auto parseBody(Json& json) {
	static std::unordered_map<std::string, b2BodyType> bodyTypes = {
		{"static", b2_staticBody},
		{"dynamic", b2_dynamicBody},
		{"kinematic", b2_kinematicBody}
	};

	auto bodyDef = b2DefaultBodyDef();
	bodyDef.type = bodyTypes[json.value("type", "static")];

	bodyDef.gravityScale = json.value("gravityScale", 1.f);

	bodyDef.angularDamping = json.value("angularDamping", 1.f);

	return bodyDef;
}

auto parsePolygon(Json& json) {
	f32 width = json["width"];
	f32 height = json["height"];

	return b2MakeBox(width * 0.5f * scale, height * 0.5f * scale);
}

auto parseCircle(Json& json) {
	b2Circle circle;
	f32 radius = json["radius"];
	circle.center = b2Vec2_zero;
	circle.radius = radius * scale;

	return circle;
}

auto parseMaterial(Json& json) {
	auto material = b2DefaultSurfaceMaterial();
	if (not json.is_null()) {
		material.friction = json.value("friction", 1.f);
		material.restitution = json.value("restitution", 1.f);
		material.rollingResistance = json.value("rollingResistance", 0.f);
	}
	return material;
}

auto parseShape(Json& json) {

	auto shapeDef = b2DefaultShapeDef();

	shapeDef.density = json.value("density", 1.f);
	shapeDef.enableHitEvents = true;
	shapeDef.enableContactEvents = true;
	shapeDef.filter.categoryBits = 1ull << json.value("layer", 0);

	shapeDef.material = parseMaterial(json["material"]);

	return shapeDef;
}

auto parseShapeType(Json& json) {
	static std::unordered_map<std::string, b2ShapeType> bodyTypes = {
		{"circle", b2_circleShape},
		{"box", b2_polygonShape},
	};

	auto val = json["type"];
	if (val.is_null()) {
		throw std::invalid_argument("\"type\" key not found");
	}
	return bodyTypes[val];
}

auto parseTexture(Json& json) {
	auto textureData = json["texture"];
	if (textureData.is_array()) {
		Color color;
		color.r = textureData[0];
		color.g = textureData[1];
		color.b = textureData[2];
		color.a = textureData[3];

		return gfx::Renderer::current()->getTextureManager()->createTexture2D(1, 1, &color);
	} else if (textureData.is_string()) {
		return loadTexture(textureData);
	}
}

std::optional<HealthData> parseHealth(Json& json) {
	auto fixedHealth = json["health"];
	if (not fixedHealth.is_null()) {
		return HealthData{fixedHealth, false};
	} else {
		auto healthMass = json["healthMass"];
		if (not healthMass.is_null()) {
			return HealthData{(float)healthMass, true};
		} else {
			return std::nullopt;
		}
	}
}

auto handleShapeType(Json& json, FxPrefab& prefab) {
	switch (prefab.shapeType) {
		case b2_circleShape:
			prefab.shapeData.circle = parseCircle(json);
			break;
		case b2_polygonShape:
			prefab.shapeData.polygon = parsePolygon(json);
			break;
		default:
			throw std::invalid_argument("invalid shape type");
	}
}

auto& loadPrefab(std::string_view name) {
	auto path = std::filesystem::path(std::format("fxPrefabs/{}.json", name));
	if (not std::filesystem::is_regular_file(path)) {
		throw std::invalid_argument(std::format("Prefab '{}' does not exist!", name));
	}

	auto&& prefab = scene::SceneManager::get()
		->currentScene()
		->domain()
		.global<UnorderedMapString<FxPrefab>>()[std::string(name)];
	prefab.json = Json::parse(std::ifstream(path));

	prefab.bodyDef = parseBody(prefab.json["body"]);
	prefab.shapeDef = parseShape(prefab.json["shape"]);
	prefab.shapeType = parseShapeType(prefab.json["shape"]);
	prefab.healthDataOpt = parseHealth(prefab.json);
	handleShapeType(prefab.json["shape"], prefab);
	prefab.texture = makePipeline(parseTexture(prefab.json));
	prefab.name = name;

	return prefab;
}

OptRef<const FxPrefab> FxPrefabManager::get(std::string_view name) {
	auto&& prefabMap = scene::SceneManager::get()->currentScene()->domain().global<UnorderedMapString<FxPrefab>>();

	auto found = prefabMap.find(name);
	if (found != prefabMap.end()) {
		return found->second;
	} else {
		try {
			return loadPrefab(name);
		} catch (std::exception& e) {
			Logger::error("{}", e.what());
			return std::nullopt;
		}
	}
}

void FxPrefabManager::clear() {
	scene::SceneManager::get()->currentScene()->domain().global<UnorderedMapString<FxPrefab>>().clear();
}
