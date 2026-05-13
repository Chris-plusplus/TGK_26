#include <LevelManager.h>
#include <Defaults.h>
#include <Utils.h>
#include <archimedes/Scene.h>
#include <ranges>
#include <DestructionSystem.h>
#include <SlingshotSystem.h>
#include <FxPrefabManager.h>
#include <nlohmann/json.hpp>
#include <archimedes/Gfx.h>

#include <box2d/base.h>
#include <box2d/box2d.h>
#include <box2d/collision.h>
#include <box2d/id.h>
#include <box2d/math_functions.h>
#include <box2d/types.h>
#include <b2World.h>
#include <ExplosionSystem.h>
#include <AccelerationSystem.h>
#include <DragonSystem.h>
#include <EndingSystem.h>
#include <ScoreSystem.h>

using namespace arch;

using Json = nlohmann::json;

void applyPrefab(b2WorldId world, Entity entity, const FxPrefab& prefab) {
	auto body = entity.addComponent(b2CreateBody(world, &prefab.bodyDef));
	b2Body_SetUserData(body, (void*)entity.handle());

	b2ShapeId shape = b2_nullShapeId;
	switch (prefab.shapeType) {
		case b2_circleShape:
			shape = entity.addComponent(b2CreateCircleShape(body, &prefab.shapeDef, &prefab.shapeData.circle));
			break;
		case b2_polygonShape:
			shape = entity.addComponent(b2CreatePolygonShape(body, &prefab.shapeDef, &prefab.shapeData.polygon));
			break;
	}
	b2Shape_SetUserData(shape, (void*)entity.handle());

	if (prefab.destrDataOpt) {
		auto&& destrData = *prefab.destrDataOpt;

		if (destrData.relativeHealth) {
			entity.addComponent<Destructible>(destrData.health * b2Body_GetMass(body), destrData.destructionPoints);
		} else {
			entity.addComponent<Destructible>(destrData.health, destrData.destructionPoints);
		}
	}

	entity.addComponent(
		scene::components::MeshComponent{
			.mesh = defaultMesh(),
			.pipeline = prefab.texture
		}
	);

	entity.addComponent<FxPrefabId>(prefab.name);
}

void applyPrefab(b2WorldId world, Entity entity, std::string_view prefabName) {
	auto prefabOpt = FxPrefabManager::get(prefabName);
	if (not prefabOpt) {
		Logger::error("prefab '{}' not found", prefabName);
		return;
	}
	applyPrefab(world, entity, *prefabOpt);
}

void parsePosition(Scene& scene, Entity entity, Json& json) {
	auto position = json.value("position", std::vector<float>{0, 0});

	auto&& t = entity.addComponent<scene::components::TransformComponent>();
	t.position = float3{position[0], position[1], 0.f};

	auto&& body = entity.getComponent<b2BodyId>();
	b2Body_SetTransform(body, b2Vec2{t.position.x * scale, t.position.y * scale}, b2Body_GetRotation(body));
}

void parseRotation(Scene& scene, Entity entity, Json& json) {
	auto rotation = glm::radians(json.value("rotation", 0.f));

	auto&& t = entity.addComponent<scene::components::TransformComponent>();
	t.rotation = glm::angleAxis(rotation, zAxis());

	auto&& body = entity.getComponent<b2BodyId>();
	b2Body_SetTransform(body, b2Body_GetPosition(body), b2MakeRot(rotation));
}

void updateScale(Scene& scene, Entity entity) {
	auto&& prefabId = entity.getComponent<FxPrefabId>().value;
	auto&& prefab = *FxPrefabManager::get(prefabId);
	auto&& shape = entity.getComponent<b2ShapeId>();

	auto&& t = entity.getComponent<scene::components::TransformComponent>();
	if (prefab.shapeType == b2_circleShape) {
		auto circle = b2Shape_GetCircle(shape);
		t.scale.x = t.scale.y = circle.radius * 2.f / scale;
	} else if (prefab.shapeType == b2_polygonShape) {
		auto polygon = b2Shape_GetPolygon(shape);
		auto width = polygon.vertices[1].x - polygon.vertices[0].x;
		auto height = polygon.vertices[2].y - polygon.vertices[1].y;

		t.scale.x = width / scale;
		t.scale.y = height / scale;
	}
}

void parseExplosion(Entity can, Json& json) {
	if (json.is_null()) {
		return;
	}

	auto&& expl = can.addComponent<Explosion>(b2DefaultExplosionDef());

	expl.explosionDef.radius = json.value("radius", 1.f) * scale;
	expl.explosionDef.falloff = json.value("falloff", 1.f) * scale;
	expl.explosionDef.impulsePerLength = json.value("strength", 1.f);
	expl.damageModifier = json.value("damageModifier", 1.f);
	expl.time = json.value("time", 0.f);

	auto& layers = json["layers"];
	if (layers.size() == 0) {
		expl.explosionDef.maskBits = SIZE_MAX;
	} else {
		expl.explosionDef.maskBits = {};
		for (auto&& layer : layers) {
			expl.explosionDef.maskBits |= 1ull << layer.get<int>();
		}
	}
}

void parseAcceleration(Entity can, Json& json) {
	if (json.is_null()) {
		return;
	}

	can.addComponent<Acceleration>(json.get<float>());
}

void parseDragon(Entity dragon, Json json) {
	if (not json.is_null()) {
		dragon.addComponent<Dragon>();
	}
}

void parseCans(Scene& scene, Json& json) {
	for (auto&& canJson : json) {
		auto&& world = scene.domain().global<b2WorldWrapper>();

		auto& prefabName = canJson["prefab"];
		if (prefabName.is_null()) {
			Logger::error("no prefab for can");
			return;
		}

		auto prefabOpt = FxPrefabManager::get(prefabName);
		if (not prefabOpt) {
			Logger::error("prefab '{}' not found", (std::string_view)prefabName);
			continue;
		}
		auto&& prefab = *prefabOpt;

		auto can = scene.newEntity();
		can.addComponent<LevelEntity>();

		applyPrefab(world.id, can, prefab);

		parsePosition(scene, can, canJson);
		updateScale(scene, can);
		parseRotation(scene, can, canJson);

		auto explJson = prefab.json.value("explosion", Json());
		parseExplosion(can, explJson);

		auto accJson = prefab.json.value("acceleration", Json());
		parseAcceleration(can, accJson);

		can.addComponent<Can>(&*FxPrefabManager::get(prefabName), prefab.json.value("points", 0.f));
	}
}

void parseObjects(Scene& scene, Json& json) {
	auto&& world = scene.domain().global<b2WorldWrapper>();

	for (auto&& [groupName, groupJson] : json.items()) {
		auto prefabName = groupJson.value("prefab", "");
		for (auto&& objectJson : groupJson["objects"]) {
			auto object = scene.newEntity();
			object.addComponent<LevelEntity>();

			const FxPrefab* prefab = nullptr;

			auto& objectPrefabName = objectJson["prefab"];
			if (objectPrefabName.is_null()) {
				if (prefabName.empty()) {
					Logger::error("Object in group '{}' without prefab!", groupName);
					continue;
				} else {
					applyPrefab(world.id, object, prefabName);
					prefab = &*FxPrefabManager::get(prefabName);
				}
			} else {
				applyPrefab(world.id, object, objectPrefabName);
				prefab = &*FxPrefabManager::get(objectPrefabName);
			}

			parsePosition(scene, object, objectJson);
			updateScale(scene, object);
			parseRotation(scene, object, objectJson);

			parseDragon(object, prefab->json.value("dragon", Json()));
		}
	}

	/*for (auto&& b : json["blocks"]) {
		if (b.is_array()) {
			parseBlocks(scene, b);
		} else {
			auto block = scene.newEntity();
			block.addComponent<LevelEntity>();

			auto prefabName = b["prefab"];
			if (prefabName.is_null()) {
				Logger::error("no prefab for can");
				return;
			}
			applyPrefab(world.id, block, prefabName);

			parsePosition(scene, block, b);
			updateScale(scene, block);
			parseRotation(scene, block, b);
		}
	}*/
}

void parseWalls(Scene& scene, Json& json) {
	auto&& world = scene.domain().global<b2WorldWrapper>();
	for (auto&& b : json["walls"]) {
		auto block = scene.newEntity();
		block.addComponent<LevelEntity>();

		auto& prefabName = b["prefab"];
		if (prefabName.is_null()) {
			Logger::error("no prefab for can");
			return;
		}
		applyPrefab(world.id, block, prefabName);

		parsePosition(scene, block, b);
		updateScale(scene, block);
		parseRotation(scene, block, b);

		auto&& t = block.getComponent<scene::components::TransformComponent>();
	}
}

void parseSlingshot(Json& json) {
	auto position = json.value("position", std::vector<float>{0, 0});

	auto slingshot = SlingshotSystem::placeSlingshot({position[0], position[1]});
	slingshot.addComponent<LevelEntity>();
	slingshot.addComponent<Slingshot>(
		0,
		json.value("canReloadTime", 1.f),
		float3{position[0], position[1], 0},
		json.value("maxPull", 100.f),
		json.value("forceMultiplier", 1.f)
	);
}

struct LevelData {
	std::string current;
	std::string next;
};

void LevelManager::loadLevel(std::string_view filename) {
	clearLevel();
	auto json = Json::parse(std::ifstream(filename.data()));
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto&& world = domain.global<b2WorldWrapper>();

	//parseTempCan(scene, json["tempCan"]);
	parseCans(scene, json["cans"]);
	parseSlingshot(json["slingshot"]);
	parseObjects(scene, json["objects"]);

	domain.global<LevelData>().current = filename;
	domain.global<LevelData>().next = json.value("next", "");

	domain.global<LevelState>() = LevelState::playing;
}

void LevelManager::nextLevel() {
	loadLevel(scene::SceneManager::get()->currentScene()->domain().global<LevelData>().next);
}

void LevelManager::reloadLevel() {
	loadLevel(scene::SceneManager::get()->currentScene()->domain().global<LevelData>().current);
}

void LevelManager::clearLevel() {
	ScoreSystem::reset();

	auto&& domain = scene::SceneManager::get()->currentScene()->domain();
	auto toKill = domain.view<LevelEntity>() | std::ranges::to<std::vector>();
	for (auto e : toKill) {
		auto shapeOpt = domain.tryGetComponent<b2ShapeId>(e);
		if (shapeOpt) {
			b2DestroyShape(*shapeOpt, false);
		}
		auto bodyOpt = domain.tryGetComponent<b2BodyId>(e);
		if (bodyOpt) {
			b2DestroyBody(*bodyOpt);
		}
		domain.kill(e);
	}
}