#include <ExplosionSystem.h>
#include <SlingshotSystem.h>
#include <ScoreSystem.h>
#include <CollisionSystem.h>
#include <b2World.h>
#include <Defaults.h>
#include <Health.h>

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
				Health* health;
			};

			std::vector<VelocityState> state;

			for (auto&& [entity, body, h] : domain.view<b2BodyId, Health>().all()) {
				state.emplace_back(entity, body, b2Body_GetLinearVelocity(body), &h);
			}

			b2World_Explode(world.id, &explosion.explosionDef);

			for (auto&& [entity, body, vel, health] : state) {
				if (health->value < 0) {
					continue;
				}

				auto velDiff = b2Body_GetLinearVelocity(body) - vel;

				auto mass = b2Body_GetMass(body);

				auto energy = explosion.damageModifier * mass * b2Dot(velDiff, velDiff);

				if (energy <= 0.f) {
					continue;
				}

				ScoreSystem::add(std::min(energy, health->value));
				if ((health->value -= energy) <= 0.f) {
					toDestroy.push_back(entity);
				}
			}

			for (auto&& entity : toDestroy) {
				b2DestroyBody(domain.getComponent<b2BodyId>(entity));
				domain.kill(entity);
			}

			b2DestroyBody(body);
			domain.kill(can);
			break;
		}
	}
}