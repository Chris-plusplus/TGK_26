#include <CollisionSystem.h>
#include <AccelerationSystem.h>
#include <SlingshotSystem.h>
#include <ExplosionSystem.h>
#include <archimedes/Scene.h>
#include <ScoreSystem.h>
#include <box2d/box2d.h>
#include <b2World.h>
#include <Health.h>

using namespace arch;

void CollisionSystem::update() {
	auto scene = scene::SceneManager::get()->currentScene();
	auto&& domain = scene->domain();
	auto&& world = domain.global<b2WorldWrapper>();

	std::vector<ecs::Entity> toDestroy;
	auto contactEvents = b2World_GetContactEvents(world.id);
	for (auto&& event : std::ranges::subrange(
		contactEvents.hitEvents,
		contactEvents.hitEvents + contactEvents.hitCount
	)) {
		auto bodyA = b2Shape_GetBody(event.shapeIdA);
		auto bodyB = b2Shape_GetBody(event.shapeIdB);

		float mA = b2Body_GetMass(bodyA);
		float mB = b2Body_GetMass(bodyB);

		float reducedMass = mA == 0 ?
			mB :
			(mB == 0 ?
				mA :
				(mA * mB) / (mA + mB));
		float e = std::max(
			b2Shape_GetRestitution(event.shapeIdA),
			b2Shape_GetRestitution(event.shapeIdA)
		);

		float energyLoss = reducedMass * 0.5f * event.approachSpeed * event.approachSpeed * (1.f - e * e);

		auto handleEntity = [&](ecs::Entity entity) {
			auto hOpt = domain.tryGetComponent<Health>(entity);
			if (hOpt and hOpt->value > 0) {
				ScoreSystem::add(std::max(0.f, std::min(energyLoss, hOpt->value)));
				if ((hOpt->value -= energyLoss) < 0) {
					toDestroy.push_back(entity);
				}
			}

			if (domain.hasComponent<Launched>(entity))
				domain.addComponent<Collided>(entity);
		};

		auto eA = (ecs::Entity)(size_t)b2Body_GetUserData(bodyA);
		auto eB = (ecs::Entity)(size_t)b2Body_GetUserData(bodyB);

		handleEntity(eA);
		handleEntity(eB);
	}
	for (auto&& entity : toDestroy) {
		if (domain.alive(entity)) {
			b2DestroyBody(domain.getComponent<b2BodyId>(entity));
			domain.kill(entity);
		}
	}
}