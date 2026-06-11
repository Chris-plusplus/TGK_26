#include <CameraSystem.h>
#include <LevelManager.h>
#include <archimedes/Input.h>
#include <Utils.h>
#include <glm/gtx/string_cast.hpp>

struct DragState {
	bool active = false;
	float2 grabWorld;
	float2 grabMouse;
	float2 startCameraPos;
} drag;

void CameraSystem::update() {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();
	auto&& levelData = domain.global<LevelData>();
	auto&& camera = domain.global<Camera>();

	bool changed = false;

	auto scroll = input::Mouse::scroll.y();
	if (scroll > 0) {
		camera.zoomIn(levelData.zoomFactor);
		if (camera.zoom() < levelData.cameraMinZoom) {
			camera.setZoom(levelData.cameraMinZoom);
		}
		changed = true;
	} else if (scroll < 0) {
		camera.zoomOut(levelData.zoomFactor);
		if (camera.zoom() > levelData.cameraMaxZoom) {
			camera.setZoom(levelData.cameraMaxZoom);
		}
		changed = true;
	}

	auto&& dragKey = input::Mouse::scroll;

	if (dragKey.pressed()) {
		drag.grabMouse = input::Mouse::pos();
		drag.grabWorld = camera.screenToWorldPos(drag.grabMouse);
		drag.startCameraPos = camera.pos();
	}
	if (dragKey.down()) {
		changed = true;
		float2 mouse = input::Mouse::pos();
		float2 currentWorld = camera.screenToWorldPos(mouse);

		auto newPos = camera.pos() + (drag.grabWorld - currentWorld);

		newPos = glm::clamp(newPos, levelData.cameraMinPosition, levelData.cameraMaxPosition);

		camera.setPos(newPos);
	}

	if (changed) {
		Logger::debug("camera state:\n  pos = {}\n  extents = {}", glm::to_string(camera.pos()), glm::to_string(camera.extents()));
	}
}