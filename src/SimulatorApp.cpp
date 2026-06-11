#include <SimulatorApp.h>
#include <archimedes/Scene.h>
#include <Defaults.h>
#include <EngineConfig.h>
#include <Utils.h>
#include <sprite/Circle.h>
#include <random>
#include <numbers>
#include <Textures.h>
#include <States.h>
#include <glm/gtx/string_cast.hpp>
#include <box2d/base.h>
#include <box2d/box2d.h>
#include <box2d/collision.h>
#include <box2d/id.h>
#include <box2d/math_functions.h>
#include <box2d/types.h>
#include <b2World.h>
#include <Button.h>
#include <LevelManager.h>
#include <DestructionSystem.h>
#include <SlingshotSystem.h>
#include <ExplosionSystem.h>
#include <AccelerationSystem.h>
#include <ScoreSystem.h>
#include <CollisionSystem.h>
#include <DespawnSystem.h>
#include <EndingSystem.h>
#include <CanAngleSystem.h>
#include <CameraSystem.h>
#include <DragonSystem.h>
#include <ButtonSystem.h>
#include <archimedes/Camera.h>
#include <archimedes/Monitor.h>

using namespace std::chrono_literals;

struct MouseControlled {
	static constexpr bool flagComponent = true;
};

void invert(std::vector<Color>& data) {
	auto i1 = data.begin();
	auto i2 = data.end() - 1;
	while (i1 < i2) {
		std::swap(*i1++, *i2--);
	}
}

Ref<gfx::Texture> makeCircleTextureWithMarker(const u32 radius, Color color, i32 thickness) {
	auto size = radius * 2;
	auto textureData = std::vector<Color>(size * size);
	auto currPx = textureData.data();
	auto radiusSqr = radius * radius;

	for (u32 j = 0; j != size; ++j) {
		float y = j + 0.5f - radius;
		float ySqr = y * y;
		for (u32 i = 0; i != size; ++i) {
			float x = i + 0.5f - radius;
			auto dist = x * x + ySqr;

			*currPx++ = f32(dist <= radiusSqr) * color;
		}
	}

	for (u32 y = radius - thickness / 2; y != radius + thickness / 2; ++y) {
		for (u32 x = radius; x != size; ++x) {
			auto&& data = textureData[y * size + x];
			data.r = 1.f - data.r;
			data.g = 1.f - data.g;
			data.b = 1.f - data.b;
		}
	}

	invert(textureData);

	return gfx::Renderer::getCurrent()->getTextureManager()->createTexture2D(size, size, textureData.data());
}

Ref<Scene> mainMenuScene;
Ref<Scene> gameScene;

SimulatorApp::~SimulatorApp() {
	mainMenuScene = nullptr;
	gameScene = nullptr;
}

void initGame() {
	gameScene = nullptr;
	gameScene = createRef<Scene>();
	scene::SceneManager::get()->changeScene(gameScene);
	auto&& domain = gameScene->domain();

	auto&& camera = getCamera();

	camera.setPos(Monitor::get()->originalSize() / 2);
	camera.setExtents(Monitor::get()->originalSize() / 2);

	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = {.x = 0, .y = -10};
	//worldDef.hitEventThreshold = 100.f * scale;
	auto worldId = b2CreateWorld(&worldDef);
	auto&& world = domain.global<b2WorldWrapper>(worldId, scale, 0.f);

	LevelManager::loadLevel("levels/1.json");

	ScoreSystem::setup();
}

struct MainMenuCanConfig {
	const FxPrefab* prefab;
	std::uniform_real_distribution<float> xDist;
	float y;
	std::uniform_real_distribution<float> angleDist;
	std::uniform_real_distribution<float> speedDist;
	std::uniform_real_distribution<float> sizeDist;
	float chance;
	float gravity;
	std::uniform_real_distribution<float> rotationSpeedDist;
};

std::vector<MainMenuCanConfig> mainMenuCanConfigs;

void SimulatorApp::init() {
	mainMenuScene = createRef<Scene>();
	scene::SceneManager::get()->changeScene(mainMenuScene);
	getWindow().toggleFullscreen();

	auto json = Json::parse(std::ifstream("settings/mainMenu.json"));

	auto&& window = getWindow();
	auto&& camera = mainMenuScene->domain().global<WindowFixedCamera>();
	camera.setPos({0, 0});
	camera.setExtents(window.size() / 2);

	{
		auto background = mainMenuScene->newEntity();
		background.addComponent(
			scene::components::TransformComponent{
				.position = {},
				.rotation = {},
				.scale = float3(window.size(), 1.f)
			}
		);
		background.addComponent(
			scene::components::MeshComponent{
				.mesh = defaultMesh(),
				.pipeline = makeScreenPipeline(parseTexture(json, "background"))
			}
		);
	}

	{
		auto button = parseButton(json, "startButton");
		button.addComponent<NamedButton>("startButton");
	}

	{
		auto&& logoJson = json["logo"];

		auto logoTexture = parseTexture(logoJson, "texture");
		auto sizeMul = logoJson.value("sizeMul", 1.f);

		auto logo = mainMenuScene->newEntity();
		logo.addComponent(
			scene::components::TransformComponent{
				.position = parseVec(logoJson, "position"),
				.rotation = {},
				.scale = float3(logoTexture->getSize().x * sizeMul, logoTexture->getSize().y * sizeMul, 1.f)
			}
		);
		logo.addComponent(
			scene::components::MeshComponent{
				.mesh = defaultMesh(),
				.pipeline = makeScreenPipeline(logoTexture)
			}
		);
	}

	for (auto&& canJson : json["cans"]) {
		auto&& canConfig = mainMenuCanConfigs.emplace_back();

		canConfig.prefab = &*FxPrefabManager::get(canJson["prefab"]);
		canConfig.xDist = std::uniform_real_distribution{canJson["x"][0].get<float>(), canJson["x"][1].get<float>()};
		canConfig.y = canJson["y"].get<float>();
		canConfig.angleDist = std::uniform_real_distribution{canJson["angle"][0].get<float>(), canJson["angle"][1].get<float>()};
		canConfig.speedDist = std::uniform_real_distribution{canJson["speed"][0].get<float>(), canJson["speed"][1].get<float>()};
		canConfig.sizeDist = std::uniform_real_distribution{canJson["size"][0].get<float>(), canJson["size"][1].get<float>()};
		canConfig.chance = canJson["chance"].get<float>();
		canConfig.gravity = canJson["gravity"].get<float>();
		canConfig.rotationSpeedDist = std::uniform_real_distribution{canJson["rotationSpeed"][0].get<float>(), canJson["rotationSpeed"][1].get<float>()};
	}
}

double score = 0;

void SimulatorApp::update() {
	auto&& currScene = scene::SceneManager::get()->currentScene();
	if (currScene == gameScene) {
		auto&& scene = currScene;
		auto&& domain = scene->domain();

		auto&& world = domain.global<b2WorldWrapper>();

		static auto tBefore = std::chrono::high_resolution_clock::now();
		const auto now = std::chrono::high_resolution_clock::now();
		const auto deltaTime = std::chrono::duration_cast<decltype(0.s)>(now - tBefore);
		tBefore = now;

		std::this_thread::sleep_for(decltype(0.s)(1.0 / 120 - deltaTime.count()));

		auto dt = 1.0 / 120;// std::min<float>(deltaTime.count(), 1.0 / 60);
		world.deltaTime = dt;

		b2World_Step(world.id, dt, 8);

		for (auto&& [entity, bodyId, t] : domain.view<b2BodyId, scene::components::TransformComponent>().all()) {
			auto bodyPos = b2Body_GetPosition(bodyId);
			auto bodyRot = b2Body_GetRotation(bodyId);

			t.position.x = bodyPos.x / scale;
			t.position.y = bodyPos.y / scale;
			t.rotation = glm::angleAxis(std::atan2(bodyRot.s, bodyRot.c), float3{0, 0, 1});
		}

		CameraSystem::update();
		ButtonSystem::update();

		for (auto&& _ : domain.view<Button::Clicked, RepeatButton>()) {
			LevelManager::reloadLevel();
			break;
		}

		SlingshotSystem::moveCanStep(dt);
		SlingshotSystem::update();
		CollisionSystem::update();
		CanAngleSystem::update();

		if (input::Keyboard::esc.pressed()) {
			scene::SceneManager::get()->changeScene(mainMenuScene);
			return;
		}

		if (input::Keyboard::arrowRight.pressed()) {
			LevelManager::nextLevel();
		}

		if (input::Keyboard::F11.pressed()) {
			getWindow().toggleFullscreen();
		}

		ExplosionSystem::update();
		AccelerationSystem::update();

		ScoreSystem::update();

		DespawnSystem::update();

		EndingSystem::update();

		/*for (auto&& [dest] : domain.view<Destructible, Dragon>().components()) {
			Logger::debug("{}", dest.health);
		}*/
	} else if (currScene == mainMenuScene) {
		static auto tBefore = std::chrono::high_resolution_clock::now();
		const auto now = std::chrono::high_resolution_clock::now();
		const auto deltaTime = std::chrono::duration_cast<decltype(0.s)>(now - tBefore);
		tBefore = now;

		std::this_thread::sleep_for(decltype(0.s)(1.0 / 120 - deltaTime.count()));

		auto rng = std::mt19937(std::random_device{}());
		CameraSystem::update();
		ButtonSystem::update();

		for (auto&& [namedButton] : currScene->domain().view<NamedButton, Button::Clicked>().components()) {
			if (namedButton.name == "startButton") {
				initGame();
				scene::SceneManager::get()->changeScene(gameScene);
			}
		}

		for (auto&& canConfig : mainMenuCanConfigs) {
			if (std::generate_canonical<float, -1>(rng) < canConfig.chance) {
				auto can = currScene->newEntity();
				can.addComponent(
					scene::components::TransformComponent{
						.position = float3(canConfig.xDist(rng), canConfig.y, -0.97),
						.rotation = {},
						.scale = float3((float2)canConfig.prefab->textureNotPipeline->getSize() * canConfig.sizeDist(rng), 1.f)
					}
				);
				can.addComponent(
					scene::components::MeshComponent{
						.mesh = defaultMesh(),
						.pipeline = makeScreenPipeline(canConfig.prefab->textureNotPipeline)
					}
				);

				auto angle = glm::radians(canConfig.angleDist(rng) + 90);
				auto speed = canConfig.speedDist(rng);
				auto rotationSpeed = glm::radians(canConfig.rotationSpeedDist(rng));
				can.addComponent<float3>(std::cos(angle) * speed, std::sin(angle) * speed, rotationSpeed);
				can.addComponent(&canConfig);
			}
		}

		std::vector<ecs::Entity> toRemove;
		for (auto&& [entity, t, velRot, canConfig] : currScene->domain().view<scene::components::TransformComponent, float3, MainMenuCanConfig*>().all()) {
			t.position.x += velRot.x;
			t.position.y += velRot.y;

			velRot.y -= canConfig->gravity;

			t.rotation = angleToQuat(glm::eulerAngles(t.rotation).z - velRot.z);

			if (t.position.y < canConfig->y) {
				toRemove.push_back(entity);
			}
		}
		for (auto&& entity : toRemove) {
			currScene->removeEntity(entity);
		}
	}
}
