#include <AccelerationSystem.h>
#include <archimedes/Scene.h>
#include <box2d/box2d.h>
#include <SlingshotSystem.h>
#include <CollisionSystem.h>
#include <Defaults.h>
#include <numbers>
#include <b2World.h>
#include <LevelManager.h>
#include <glm/gtx/string_cast.hpp>

using namespace arch;

struct FoamParticle {
	float time = 0;
	float2 vel = {};
	float speedLoss = 0;
};

void AccelerationSystem::update() {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto&& world = domain.global<b2WorldWrapper>();
	auto toRemove = domain.view<Acceleration, Collided>()
		| std::ranges::to<std::vector>();

	for (auto&& entity : toRemove) {
		domain.removeComponent<Acceleration>(entity);
	}
	toRemove.clear();

	for (auto&& [entity, t, foam] : domain.view<scene::components::TransformComponent, FoamParticle>().all()) {
		t.position.x += foam.vel.x;
		t.position.y += foam.vel.y;

		foam.vel *= foam.speedLoss;

		if ((foam.time -= world.deltaTime) <= 0.f) {
			toRemove.push_back(entity);
		}
	}
	for (auto&& entity : toRemove) {
		domain.kill(entity);
	}

	for (auto&& [can, acc, t, body] : domain.view<Acceleration, scene::components::TransformComponent, b2BodyId>(exclude<Can>).all()) {
		if (acc.value != 0) {
			if (input::Mouse::left.pressed()) {
				b2Body_SetLinearVelocity(body, acc.value * b2Normalize(b2Body_GetLinearVelocity(body)));
				acc.value = 0;

				if (acc.textureOnUse) {
					domain.getComponent<scene::components::MeshComponent>(can).pipeline = std::move(acc.textureOnUse);
				}
			}
		} else {
			for (u32 i = 0; i != acc.newFoams; ++i) {
				auto foam = scene.newEntity();

				foam.addComponent(
					scene::components::MeshComponent{
						.mesh = defaultMesh(),
						.pipeline = acc.foamPipeline
					}
				);

				float exhaustAngle = std::numbers::pi_v<float> +glm::eulerAngles(t.rotation).z;

				foam.addComponent(
					scene::components::TransformComponent{
						.position = t.position + t.scale * 0.5f * float3{std::cos(exhaustAngle), std::sin(exhaustAngle), 0},
						.rotation = glm::angleAxis(acc.foamRotationDist(acc.rng), float3{0, 0, 1}),
						.scale = float3(acc.foamTexture->getWidth(), acc.foamTexture->getHeight(), 0)
					}
				);

				float angle = acc.foamAngleDist(acc.rng) + exhaustAngle;

				foam.addComponent<FoamParticle>(
					acc.foamTime,
					acc.foamVelocityDist(acc.rng) * float2 {
					std::cos(angle), std::sin(angle)
				},
					acc.foamSpeedLoss
				);
				foam.addComponent<LevelEntity>();
			}
		}
	}
}