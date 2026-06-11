#include <SlingshotSystem.h>
#include <CollisionSystem.h>
#include <archimedes/Scene.h>
#include <Defaults.h>
#include <Utils.h>

Entity SlingshotSystem::placeSlingshot(float2 pos, std::string_view texturePath) {
	auto&& scene = *scene::SceneManager::get()->currentScene();

	auto slingshot = scene.newEntity();
	auto texture = loadTexture(texturePath);
	auto pipeline = makeCameraPipeline(texture);

	slingshot.addComponent(
		scene::components::TransformComponent{
			.position = {pos.x, pos.y - texture->getHeight() / 2, 0},
			.rotation = angleToQuat(0),
			.scale = {texture->getWidth(), texture->getHeight(), 0}
		}
	);
	slingshot.addComponent(
		scene::components::MeshComponent{
			.mesh = defaultMesh(),
			.pipeline = pipeline
		}
	);

	return slingshot;
}

ecs::Entity nextCan(Scene& scene) {
	for (auto&& can : scene.domain().view<Can>()) {
		return can;
	}
	return ecs::nullEntity;
}

void SlingshotSystem::moveCanStep(float dt) {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto&& [entity, slingshot] = domain.view<Slingshot>().all().front();

	if (slingshot.state == slingshot.waiting) {
		slingshot.canReloadTimePassed += dt;
		if (slingshot.canReloadTimePassed > slingshot.canReloadTime) {
			auto canEntity = nextCan(scene);
			if (canEntity != ecs::nullEntity) {
				slingshot.state = slingshot.reloading;

				auto&& canos = domain.addComponent<CanOnSlingshot>(canEntity);
				auto&& t = domain.getComponent<scene::components::TransformComponent>(canEntity);
				canos.posBegin = t.position;

				auto body = domain.getComponent<b2BodyId>(canEntity);
				b2Body_SetType(body, b2_staticBody);
			} else {
				slingshot.state = slingshot.empty;
			}
		}
	} else if (slingshot.state == slingshot.reloading) {
		auto&& [entity, can, canos, t, body] = domain.view<Can, CanOnSlingshot, scene::components::TransformComponent, b2BodyId>().all().front();

		canos.progress = std::min(canos.progress + canos.deltaProgressPerSec * dt, 1.f);

		t.position = glm::mix(canos.posBegin, slingshot.centerPos, canos.progress);
		syncBodyToTransform(body, t);
		b2Body_SetType(body, b2_staticBody);
		if (canos.progress == 1) {
			slingshot.state = slingshot.loaded;
		}
	}
}

void SlingshotSystem::update() {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto&& camera = domain.global<Camera>();
	auto&& [slingshotEntity, slingshot] = domain.view<Slingshot>().all().front();

	auto mouseWorldPos = camera.screenToWorldPos(input::Mouse::pos());

	if (slingshot.state == slingshot.loaded) {
		auto&& [canEntity, can, canos, t, body, shape] = domain.view<Can, CanOnSlingshot, scene::components::TransformComponent, b2BodyId, b2ShapeId>().all().front();
		auto mouseIn = [&t, mouseWorldPos] {
			auto distance = float3(mouseWorldPos, 0) - t.position;
			distance *= distance;
			return distance.x + distance.y < t.scale.x * t.scale.x / 4.f;
		};

		if (input::Mouse::left.pressed() and mouseIn()) {
			slingshot.state = slingshot.dragged;
		}
	} else if (slingshot.state == slingshot.dragged) {
		auto&& [canEntity, can, canos, t, body] = domain.view<Can, CanOnSlingshot, scene::components::TransformComponent, b2BodyId>().all().front();
		if (input::Mouse::left.down()) {
			auto dpos = float3(mouseWorldPos, 0) - slingshot.centerPos;
			auto length = std::min(glm::length(dpos), slingshot.maxPull);

			dpos = length * glm::normalize(dpos);

			t.position = slingshot.centerPos + dpos;
			syncBodyToTransform(body, t);
			b2Body_SetType(body, b2_staticBody);

			for (auto&& [band, bandT] : domain.view<Band, scene::components::TransformComponent>().all()) {
				auto dposBand = (t.position + glm::normalize(dpos) * t.scale.x * 0.5f) - bandT.position;
				auto angle = std::atan2(dposBand.y, dposBand.x) + 3.14159265f / 2.f;

				bandT.scale.y = glm::length(dposBand) * 2.f;
				bandT.rotation = glm::angleAxis(angle, float3{0, 0, 1});
			}
		} else if (input::Mouse::left.released()) {
			b2Body_SetType(body, b2_dynamicBody);
			auto force = b2Body_GetMass(body) * slingshot.forceMultiplier * (b2Vec2{slingshot.centerPos.x * scale, slingshot.centerPos.y * scale} - b2Body_GetPosition(body));

			b2Body_ApplyLinearImpulseToCenter(body, force, true);
			domain.removeComponent<CanOnSlingshot>(canEntity);
			domain.removeComponent<Can>(canEntity);

			slingshot.state = slingshot.waiting;
			slingshot.canReloadTimePassed = 0;

			domain.addComponent<Launched>(canEntity);
		}
	}
}