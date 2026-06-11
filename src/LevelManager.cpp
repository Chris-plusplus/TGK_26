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
#include <CollisionSystem.h>

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
#include <ButtonSystem.h>
#include <glm/gtx/string_cast.hpp>
#include <generator>

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

void parsePosition(Scene& scene, Entity entity, Json& json, float3 delta = {}) {
	auto&& t = entity.addComponent<scene::components::TransformComponent>();
	t.position = parseVec(json, "position") + delta;

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

	auto&& acc = can.addComponent<Acceleration>();
	acc.value = json.value("value", 0.f);
	acc.textureOnUse = makeCameraPipeline(parseTexture(json, "textureOnUse"));
	acc.foamTexture = parseTexture(json, "foamTexture");
	acc.foamPipeline = makeCameraPipeline(acc.foamTexture);
	acc.rng = std::minstd_rand(std::random_device{}());
	acc.foamVelocityDist = decltype(acc.foamVelocityDist)(json.value("foamMinVelocity", 0.f), json.value("foamMaxVelocity", 0.f));
	auto maxAngle = glm::radians(json.value("foamMaxAngle", 0.f));
	acc.foamAngleDist = decltype(acc.foamAngleDist)(-maxAngle, maxAngle);
	acc.foamRotationDist = decltype(acc.foamRotationDist)(glm::radians(0.f), glm::radians(360.f));
	acc.foamTime = json.value("foamTime", 0.f);
	acc.newFoams = json.value("newFoams", 0u);
	acc.foamSpeedLoss = json.value("foamSpeedLoss", 0.f);
}

void parseDragon(Entity dragon, Json json) {
	if (not json.is_null()) {
		dragon.addComponent<Dragon>();
	}
}

void parseDamageToOthers(Entity entity, Json& json) {
	if (json.is_null()) {
		return;
	}

	entity.addComponent<DamageToOthers>(json.value("damageToOthersMultiplier", 1.f));
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
		parseDamageToOthers(can, canJson);

		auto explJson = prefab.json.value("explosion", Json());
		parseExplosion(can, explJson);

		auto accJson = prefab.json.value("acceleration", Json());
		parseAcceleration(can, accJson);

		can.addComponent<Can>(&*FxPrefabManager::get(prefabName), prefab.json.value("points", 0.f));
	}
}

std::generator<Json&> getObjectList(Json& levelObjectsJson, std::string_view group) {
	auto& objectsJson = levelObjectsJson[group]["objects"];
	if (objectsJson.is_string()) {
		auto otherGroup = objectsJson.get<std::string>();
		auto recursive = getObjectList(levelObjectsJson, otherGroup);
		co_yield std::ranges::elements_of(recursive);
	} else if (objectsJson.is_array()) {
		for (auto&& objectJson : objectsJson) {
			if (objectJson.is_string()) {
				auto otherGroup = objectJson.get<std::string>();
				auto recursive = getObjectList(levelObjectsJson, otherGroup);
				co_yield std::ranges::elements_of(recursive);
			} else {
				co_yield objectJson;
			}
		}
	} else {
		Logger::debug("'objects' of group '{}' is invalid", group);
	}
}

void parseObjects(Scene& scene, Json& json, Json& levelJson) {
	auto&& world = scene.domain().global<b2WorldWrapper>();

	for (auto&& [groupName, groupJson] : json.items()) {
		auto prefabName = groupJson.value("prefab", "");
		auto basePosition = parseVec(groupJson, "basePosition", false);

		auto makeObject = [&](Json& objectJson) {
			auto object = scene.newEntity();
			object.addComponent<LevelEntity>();

			const FxPrefab* prefab = nullptr;

			auto& objectPrefabName = objectJson["prefab"];
			if (objectPrefabName.is_null()) {
				if (prefabName.empty()) {
					Logger::error("Object in group '{}' without prefab!", groupName);
					return;
				} else {
					applyPrefab(world.id, object, prefabName);
					prefab = &*FxPrefabManager::get(prefabName);
				}
			} else {
				applyPrefab(world.id, object, objectPrefabName);
				prefab = &*FxPrefabManager::get(objectPrefabName);
			}

			parsePosition(scene, object, objectJson, basePosition);
			updateScale(scene, object);
			parseRotation(scene, object, objectJson);
			parseDamageToOthers(object, objectJson);

			parseDragon(object, prefab->json.value("dragon", Json()));
		};

		for (auto&& objectJson : getObjectList(levelJson["objects"], groupName)) {
			makeObject(objectJson);
		}
	}
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
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto position = json.value("position", std::vector<float>{0, 0});

	auto slingshot = SlingshotSystem::placeSlingshot({position[0], position[1]}, json["texture"]);
	slingshot.addComponent<LevelEntity>();
	auto&& slingshotC = slingshot.addComponent<Slingshot>(
		0,
		json.value("canReloadTime", 1.f),
		float3{position[0], position[1], 0},
		json.value("maxPull", 100.f),
		json.value("forceMultiplier", 1.f)
	);
	auto bandTexture = loadTexture(json["bandTexture"]);
	auto bandPipeline = makeCameraPipeline(bandTexture);

	for (auto&& bandPos : json["bandPosition"]) {
		float dx = bandPos[0];
		float dy = bandPos[1];

		auto band = scene.newEntity();

		band.addComponent(
			scene::components::MeshComponent{
				.mesh = defaultMesh(),
				.pipeline = bandPipeline
			}
		);
		band.addComponent(
			scene::components::TransformComponent{
				.position = slingshotC.centerPos + float3{dx, dy, 0},
				.rotation = {},
				.scale = {10, 0, 1}
			}
		);
		band.addComponent<Band>();
		band.addComponent<LevelEntity>();
	}
}

void addBackground(Json& json) {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();

	auto background = scene.newEntity();

	auto&& texture = parseTexture(json, "background");
	background.addComponent(
		scene::components::MeshComponent{
			.mesh = defaultMesh(),
			.pipeline = makeScreenPipeline(texture)
		}
	);
	auto&& monitor = *Monitor::get();
	background.addComponent(
		scene::components::TransformComponent{
			.position = float3(monitor.originalSize() / 2, 0.f),
			.rotation = {},
			.scale = float3(texture->getWidth(), texture->getHeight(), 1)
		}
	);
}

void LevelManager::loadLevel(std::string_view filename) {
	if (filename.empty()) return;

	clearLevel();
	FxPrefabManager::clear();

	auto json = Json::parse(std::ifstream(filename.data()));
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto&& camera = domain.global<Camera>();
	auto&& world = domain.global<b2WorldWrapper>();

	addBackground(json);

	//parseTempCan(scene, json["tempCan"]);
	parseCans(scene, json["cans"]);
	parseSlingshot(json["slingshot"]);
	parseObjects(scene, json["objects"], json);

	auto&& cameraJson = json["camera"];
	auto&& cameraPos = cameraJson["position"];
	camera.setPos({cameraPos[0], cameraPos[1]});
	auto&& cameraExtents = cameraJson["extents"];
	camera.setExtents({cameraExtents[0], cameraExtents[1]});

	auto&& levelData = domain.global<LevelData>();
	levelData.current = filename;
	levelData.next = json.value("next", "");
	levelData.cameraMinZoom = cameraJson["minZoom"];
	levelData.cameraMaxZoom = cameraJson["maxZoom"];
	levelData.zoomFactor = cameraJson.value("zoomFactor", 1.1f);

	auto tempJson = cameraJson["minPosition"];
	levelData.cameraMinPosition = float2(tempJson[0], tempJson[1]);

	tempJson = cameraJson["maxPosition"];
	levelData.cameraMaxPosition = float2(tempJson[0], tempJson[1]);

	//Logger::debug("{} {} {} {}", levelData.cameraMinZoom, levelData.cameraMaxZoom, glm::to_string(levelData.cameraMinPosition), glm::to_string(levelData.cameraMaxPosition));

	domain.global<LevelState>() = LevelState::playing;

	// repeat button
	auto repeatButtonJson = Json::parse(std::ifstream("settings/repeatButton.json"));
	auto repeatButton = parseButton(repeatButtonJson, "repeatButton");
	repeatButton.addComponent<LevelEntity>();
	repeatButton.addComponent<RepeatButton>();
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