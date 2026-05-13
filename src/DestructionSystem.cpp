#include <DestructionSystem.h>
#include <ScoreSystem.h>
#include <box2d/box2d.h>

void DestructionSystem::update() {
	auto&& domain = scene::SceneManager::get()->currentScene()->domain();

	std::vector<ecs::Entity> toKill;
	for (auto&& [entity, destructible] : domain.view<Destructible>().all()) {
		if (destructible.damage <= 0) {
			continue;
		}

		ScoreSystem::add(std::min(destructible.damage, destructible.health));
		if ((destructible.health -= destructible.damage) <= 0.f) {
			ScoreSystem::add(destructible.destructionPoints);
			toKill.push_back(entity);
		}

		destructible.damage = 0;
	}

	for (auto&& entity : toKill) {
		if (domain.alive(entity)) {
			b2DestroyBody(domain.getComponent<b2BodyId>(entity));
			domain.kill(entity);
		}
	}
}