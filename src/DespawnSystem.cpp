#include <DespawnSystem.h>
#include <SlingshotSystem.h>
#include <archimedes/Scene.h>
#include <box2d/box2d.h>
#include <Utils.h>

using namespace arch;

void DespawnSystem::update() {
	std::vector<ecs::Entity> toKill;
	auto&& domain = scene::SceneManager::get()->currentScene()->domain();
	for (auto&& [entity, body] : domain.view<b2BodyId, Launched>().all()) {
		if (not isMoving(body)) {
			toKill.push_back(entity);
		}
	}

	for (auto&& e : toKill) {
		b2DestroyBody(domain.getComponent<b2BodyId>(e));
		domain.kill(e);
	}
}