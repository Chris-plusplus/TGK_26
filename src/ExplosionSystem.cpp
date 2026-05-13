#include <ExplosionSystem.h>
#include <SlingshotSystem.h>
#include <ScoreSystem.h>
#include <CollisionSystem.h>
#include <b2World.h>
#include <Defaults.h>
#include <DestructionSystem.h>

void ExplosionSystem::update() {
	std::vector<ecs::Entity> toDestroy;
	auto&& domain = scene::SceneManager::get()->currentScene()->domain();
	auto&& world = domain.global<b2WorldWrapper>();
	for (auto&& [can, explosion, t, body, shape] : domain.view<Explosion, scene::components::TransformComponent, b2BodyId, b2ShapeId>(exclude<Can>).all()) {
		bool timesUp = false;
		if (auto c = domain.tryGetComponent<Collided>(can)) {
			timesUp = explosion.time - c->timeSince < 0;
		}

		if (input::Mouse::left.pressed() or timesUp) {
			explosion.explosionDef.position = b2Vec2{t.position.x * scale, t.position.y * scale};

			struct VelocityState {
				ecs::Entity entity;
				b2BodyId body;
				b2Vec2 vel;
				Destructible* health;
			};

			std::vector<VelocityState> state;

			for (auto&& [entity, body, h] : domain.view<b2BodyId, Destructible>().all()) {
				state.emplace_back(entity, body, b2Body_GetLinearVelocity(body), &h);
			}

			b2World_Explode(world.id, &explosion.explosionDef);

			for (auto&& [entity, body, vel, destructible] : state) {
				auto velDiff = b2Body_GetLinearVelocity(body) - vel;

				auto mass = b2Body_GetMass(body);

				auto energy = explosion.damageModifier * mass * b2Dot(velDiff, velDiff);

				if (energy <= 0.f) {
					continue;
				}

				destructible->damage += energy;
			}

			b2DestroyBody(body);
			domain.kill(can);
			break;
		}
	}
	DestructionSystem::update();
}