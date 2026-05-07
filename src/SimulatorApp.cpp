#include <SimulatorApp.h>
#include <archimedes/Scene.h>
#include <Defaults.h>
#include <EngineConfig.h>
#include <Utils.h>
#include <sprite/Circle.h>
#include <Cell.h>
#include <CellSystem.h>
#include <CellMember.h>
#include <Grid.h>
#include <SimConfig.h>
#include <random>
#include <numbers>
#include <Textures.h>
#include <States.h>
#include <box2d/base.h>
#include <box2d/box2d.h>
#include <box2d/collision.h>
#include <box2d/id.h>
#include <box2d/math_functions.h>
#include <box2d/types.h>
#include <b2World.h>
#include <Button.h>
#include <LevelManager.h>
#include <Health.h>
#include <SlingshotSystem.h>
#include <ExplosionSystem.h>
#include <AccelerationSystem.h>
#include <ScoreSystem.h>
#include <CollisionDamageSystem.h>

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

void SimulatorApp::init() {
	// make scene
	glfwSetWindowPos(gfx::Renderer::getCurrent()->getWindow()->get(), 50, 50);
	Ref<Scene> scene = createRef<Scene>();
	scene::SceneManager::get()->changeScene(scene);
	auto&& domain = scene->domain();

	auto&& textures = scene->domain().global<Textures>();
	auto radius = std::max(simConfig.infectionRadius, 1.f);
	textures.susceptible = makePipeline(loadTexture("textures/smashed_can.png"));
	textures.infected = makePipeline(makeCircleTextureWithMarker(100, {1.f, 0.f, 0.f, 1.f}, 10));
	textures.removed = makePipeline(makeCircleTexture(100, {0.9f, 0.9f, 0.9f, 1.f}));

	auto make1pxTexture = [](Color color) {
		return gfx::Renderer::getCurrent()->getTextureManager()->createTexture2D(1, 1, &color);
	};

	auto boxTexture = makePipeline(make1pxTexture({0.5, 0.5, 0.5, 1}));

	auto mesh = defaultMesh();

	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = {.x = 0, .y = -10};
	worldDef.hitEventThreshold = 100.f * scale;
	auto worldId = b2CreateWorld(&worldDef);
	auto&& world = domain.global<b2WorldWrapper>(worldId, scale, 0.f);

	LevelManager::loadLevel("levels/test.json");

	ScoreSystem::setup();
}

double score = 0;

void SimulatorApp::update() {
	auto scene = scene::SceneManager::get()->currentScene();
	auto&& domain = scene->domain();

	auto&& world = domain.global<b2WorldWrapper>();

	static auto tBefore = std::chrono::high_resolution_clock::now();
	const auto now = std::chrono::high_resolution_clock::now();
	const auto deltaTime = std::chrono::duration_cast<decltype(0.s)>(now - tBefore);
	tBefore = now;

	std::this_thread::sleep_for(decltype(0.s)(1.0 / 120 - deltaTime.count()));

	auto dt = 1.0 / 120;// std::min<float>(deltaTime.count(), 1.0 / 60);

	b2World_Step(world.id, dt, 8);

	CollisionDamageSystem::update();

	for (auto&& [entity, bodyId, t] : domain.view<b2BodyId, scene::components::TransformComponent>().all()) {
		auto bodyPos = b2Body_GetPosition(bodyId);
		auto bodyRot = b2Body_GetRotation(bodyId);

		t.position.x = bodyPos.x / scale;
		t.position.y = bodyPos.y / scale;
		t.rotation = glm::angleAxis(std::atan2(bodyRot.s, bodyRot.c), float3{0, 0, 1});
	}

	SlingshotSystem::moveCanStep(dt);
	SlingshotSystem::update();

	if (input::Keyboard::esc.pressed()) {
		ScoreSystem::reset();
		LevelManager::loadLevel("levels/test.json");
	}

	ExplosionSystem::update();
	AccelerationSystem::update();

	ScoreSystem::update();
}
