#include <AccelerationSystem.h>
#include <archimedes/Scene.h>
#include <box2d/box2d.h>
#include <SlingshotSystem.h>
#include <CollisionSystem.h>

using namespace arch;

void AccelerationSystem::update() {
	auto&& domain = scene::SceneManager::get()->currentScene()->domain();
	for (auto&& [can, acc, t, body] : domain.view<Acceleration, scene::components::TransformComponent, b2BodyId>(exclude<Can>).all()) {
		if (domain.hasComponent<Collided>(can)) {
			domain.removeComponent<Acceleration>(can);
			break;
		} else if (input::Mouse::left.pressed()) {
			b2Body_SetLinearVelocity(body, acc.value * b2Normalize(b2Body_GetLinearVelocity(body)));
			auto tex = std::move(domain.removeComponent<Acceleration>(can, moveFlag).textureOnHit);
			if (tex) {
				domain.getComponent<scene::components::MeshComponent>(can).pipeline = std::move(tex);
			}
			break;
		}
	}
}