#include <CanAngleSystem.h>
#include <SlingshotSystem.h>
#include <Defaults.h>
#include <CollisionSystem.h>
#include <archimedes/Camera.h>

void CanAngleSystem::update() {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto&& camera = domain.global<Camera>();
	auto&& [entity, slingshot] = domain.view<Slingshot>().all().front();

	if (slingshot.state == slingshot.dragged) {
		if (input::Mouse::left.down()) {
			auto&& [canEntity, can, canos, t, body] = domain.view<Can, CanOnSlingshot, scene::components::TransformComponent, b2BodyId>().all().front();
			auto dpos = slingshot.centerPos - float3(camera.screenToWorldPos(input::Mouse::pos()), 0);

			float angle = std::atan2(dpos.y, dpos.x);

			t.rotation = glm::angleAxis(angle, zAxis());
			b2Body_SetTransform(body, b2Body_GetPosition(body), b2MakeRot(angle));
		}
	}

	for (auto&& [t, body] : domain.view<scene::components::TransformComponent, b2BodyId, Launched>(exclude<Collided>).components()) {
		auto velNorm = b2Normalize(b2Body_GetLinearVelocity(body));

		auto angle = std::atan2(velNorm.y, velNorm.x);

		t.rotation = glm::angleAxis(angle, zAxis());
		b2Body_SetTransform(body, b2Body_GetPosition(body), b2MakeRot(angle));
	}
}