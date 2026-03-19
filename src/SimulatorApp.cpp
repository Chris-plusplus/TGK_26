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

constexpr f32 scale = 1 / 128.f;

using namespace std::chrono_literals;

struct MouseControlled {
	static constexpr bool flagComponent = true;
};

struct Movement {
	f32 angle;
	f32 deltaAngle;
};

struct b2WorldWrapper {
	b2WorldId world = b2_nullWorldId;

	operator b2WorldId& () {
		return world;
	}

	~b2WorldWrapper() {
		b2DestroyWorld(world);
		world = b2_nullWorldId;
	}
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
	textures.susceptible = makePipeline(makeCircleTexture(100, {0.f, 1.f, 0.f, 1.f}));
	textures.infected = makePipeline(makeCircleTextureWithMarker(100, {1.f, 0.f, 0.f, 1.f}, 10));
	textures.removed = makePipeline(makeCircleTexture(100, {0.9f, 0.9f, 0.9f, 1.f}));

	auto make1pxTexture = [](Color color) {
		return gfx::Renderer::getCurrent()->getTextureManager()->createTexture2D(1, 1, &color);
	};

	auto boxTexture = makePipeline(make1pxTexture({0.5, 0.5, 0.5, 1}));

	auto mesh = defaultMesh();

	b2WorldDef worldDef = b2DefaultWorldDef();
	worldDef.gravity = {.x = 0, .y = -10};
	auto&& world = domain.global<b2WorldWrapper>(b2CreateWorld(&worldDef));

	{
		b2BodyDef groundBodyDef = b2DefaultBodyDef();
		groundBodyDef.position = {.x = (f32)windowWidth / 2 * scale, .y = -50 * scale};
		groundBodyDef.type = b2_staticBody;
		b2BodyId groundBody = b2CreateBody(world, &groundBodyDef);

		b2Polygon groundBox = b2MakeBox(windowWidth / 2 * scale, 50 * scale);
		b2ShapeDef groundShapeDef = b2DefaultShapeDef();
		auto groundShape = b2CreatePolygonShape(groundBody, &groundShapeDef, &groundBox);
	}
	{
		b2BodyDef bodyDef = b2DefaultBodyDef();
		bodyDef.type = b2_dynamicBody;
		bodyDef.position = {.x = windowWidth / 2.0f * scale, .y = 500.0f * scale};
		b2BodyId bodyId = b2CreateBody(world, &bodyDef);

		b2Circle circleShape;
		circleShape.center = {};
		circleShape.radius = 50 * scale;

		b2ShapeDef shapeDef = b2DefaultShapeDef();
		auto material = b2DefaultSurfaceMaterial();
		material.restitution = 0.5;
		shapeDef.material = material;
		b2CreateCircleShape(bodyId, &shapeDef, &circleShape);

		auto circle = scene->newEntity();

		circle.addComponent(
			scene::components::TransformComponent{
				.position = {bodyDef.position.x / scale, bodyDef.position.y / scale, 0},
				.rotation = {0, 0, 0, 1},
				.scale = {circleShape.radius * 2 / scale, circleShape.radius * 2 / scale, 0}
			}
		);
		circle.addComponent(
			scene::components::MeshComponent{
				.mesh = mesh,
				.pipeline = textures.infected
			}
		);

		circle.addComponent<b2BodyId>(bodyId);
	}
	{
		b2BodyDef boxBodyDef = b2DefaultBodyDef();
		boxBodyDef.position = {.x = ((f32)windowWidth / 2 + 60) * scale, .y = 200 * scale};
		boxBodyDef.rotation = {.c = std::cos(glm::radians(-30.f)), .s = std::sin(glm::radians(-30.f))};
		boxBodyDef.type = b2_dynamicBody;
		b2BodyId body = b2CreateBody(world, &boxBodyDef);

		b2Polygon groundBox = b2MakeBox(50 * scale, 50 * scale);
		b2ShapeDef groundShapeDef = b2DefaultShapeDef();
		groundShapeDef.material.restitution = 0.75;
		auto groundShape = b2CreatePolygonShape(body, &groundShapeDef, &groundBox);

		auto box = scene->newEntity();

		box.addComponent(
			scene::components::TransformComponent{
				.position = {boxBodyDef.position.x / scale, boxBodyDef.position.y / scale, 0},
				.rotation = {0, 0, 0, 1},
				.scale = {100, 100, 0}
			}
		);
		box.addComponent(
			scene::components::MeshComponent{
				.mesh = mesh,
				.pipeline = boxTexture
			}
		);

		box.addComponent<b2BodyId>(body);
	}
}

void SimulatorApp::update() {
	auto scene = scene::SceneManager::get()->currentScene();
	auto&& domain = scene->domain();

	auto world = domain.global<b2WorldWrapper>().world;

	static auto tBefore = std::chrono::high_resolution_clock::now();
	const auto now = std::chrono::high_resolution_clock::now();
	const auto deltaTime = std::chrono::duration_cast<decltype(0.s)>(now - tBefore);
	tBefore = now;

	b2World_Step(world, deltaTime.count(), 4);

	for (auto&& [entity, bodyId, t] : domain.view<b2BodyId, scene::components::TransformComponent>().all()) {
		auto bodyPos = b2Body_GetPosition(bodyId);
		auto bodyRot = b2Body_GetRotation(bodyId);

		t.position.x = bodyPos.x / scale;
		t.position.y = bodyPos.y / scale;
		t.rotation = glm::angleAxis(std::atan2(bodyRot.s, bodyRot.c), float3{0, 0, 1});
	}
}
