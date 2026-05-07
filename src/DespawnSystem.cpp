#include <DespawnSystem.h>
#include <SlingshotSystem.h>
#include <archimedes/Scene.h>
#include <box2d/box2d.h>

using namespace arch;

void DespawnSystem::update() {
	std::vector<ecs::Entity> toKill;
	auto&& domain = scene::SceneManager::get()->currentScene()->domain();
	for (auto&& [entity, body] : domain.view<b2BodyId, Launched>().all()) {
		auto vel = b2Body_GetLinearVelocity(body);
		constexpr auto eps = std::numeric_limits<float>::epsilon();
		if (glm::abs(vel.x) < eps and glm::abs(vel.y) < eps and glm::abs(b2Body_GetAngularVelocity(body)) < eps) {
			toKill.push_back(entity);
		}
	}

	for (auto&& e : toKill) {
		b2DestroyBody(domain.getComponent<b2BodyId>(e));
		domain.kill(e);
	}
}