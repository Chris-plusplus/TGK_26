#include <CollisionSystem.h>
#include <AccelerationSystem.h>
#include <SlingshotSystem.h>
#include <ExplosionSystem.h>
#include <archimedes/Scene.h>
#include <ScoreSystem.h>
#include <box2d/box2d.h>
#include <b2World.h>
#include <DestructionSystem.h>

using namespace arch;

void CollisionSystem::update() {
	auto scene = scene::SceneManager::get()->currentScene();
	auto&& domain = scene->domain();
	auto&& world = domain.global<b2WorldWrapper>();

	for (auto&& [collided] : domain.view<Collided>().components()) {
		collided.timeSince += world.deltaTime;
	}

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
			b2Shape_GetRestitution(event.shapeIdB)
		);

		float energyLoss = reducedMass * 0.5f * event.approachSpeed * event.approachSpeed * (1.f - e * e);

		auto handleEntity = [&](ecs::Entity entity, float damageMultiplier) {
			if (auto dOpt = domain.tryGetComponent<Destructible>(entity)) {
				dOpt->damage += energyLoss * damageMultiplier;
			}

			if (domain.hasComponent<Launched>(entity))
				domain.addComponent<Collided>(entity, 0);
		};

		auto eA = (ecs::Entity)(size_t)b2Body_GetUserData(bodyA);
		auto eB = (ecs::Entity)(size_t)b2Body_GetUserData(bodyB);

		auto aToBDamageMul = 1.f;
		auto bToADamageMul = 1.f;
		if (auto aDamageToOthersMul = domain.tryGetComponent<DamageToOthers>(eA)) {
			aToBDamageMul = aDamageToOthersMul->multiplier;
		}
		if (auto bDamageToOthersMul = domain.tryGetComponent<DamageToOthers>(eB)) {
			bToADamageMul = bDamageToOthersMul->multiplier;
		}

		handleEntity(eA, bToADamageMul);
		handleEntity(eB, aToBDamageMul);
	}
	DestructionSystem::update();
}