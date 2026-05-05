#include <AccelerationSystem.h>
#include <archimedes/Scene.h>
#include <box2d/box2d.h>
#include <SlingshotSystem.h>

using namespace arch;

void AccelerationSystem::update() {
	if (input::Mouse::left.pressed()) {
		auto&& domain = scene::SceneManager::get()->currentScene()->domain();
		for (auto&& [can, acc, t, body] : domain.view<Acceleration, scene::components::TransformComponent, b2BodyId>(exclude<Can>).all()) {
			auto velBefore = b2Length(b2Body_GetLinearVelocity(body));

			b2Body_SetLinearVelocity(body, acc.value * b2Normalize(b2Body_GetLinearVelocity(body)));

			auto velAfter = b2Length(acc.value * b2Normalize(b2Body_GetLinearVelocity(body)));

			Logger::debug("\nacc: {:.2f} -> {:.2f}", velBefore, velAfter);

			domain.removeComponent<Acceleration>(can);

			break;
		}
	}
}