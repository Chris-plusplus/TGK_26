#include <Button.h>
#include <ButtonSystem.h>
#include <Utils.h>
#include <Defaults.h>

void ButtonSystem::update() {
	auto&& domain = scene::SceneManager::get()->currentScene()->domain();

	for (auto&& toRemove : domain.view<Button::Clicked>() | std::ranges::to<std::vector>()) {
		domain.removeComponent<Button::Clicked>(toRemove);
	}

	auto leftPressed = input::Mouse::left.released();

	auto mousePos = input::Mouse::pos();
	for (auto&& [entity, t, button] : domain.view<scene::components::TransformComponent, Button>().all()) {
		bool anyKeyPressed = [&] -> bool {
			for (auto&& keycode : button.keycodes | std::views::take_while([](auto keycode) { return keycode != 0; })) {
				if (input::Key::get(keycode).pressed()) {
					return true;
				}
			}

			return false;
		}();

		if (anyKeyPressed) {
			domain.addComponent<Button::Clicked>(entity);
			continue;
		}

		if (leftPressed) {
			auto&& camera = *button.camera;

			auto bottomLeft = t.position - t.scale / 2.f;
			auto topRight = t.position + t.scale / 2.f;
			auto mouseWorldPos = camera.screenToWorldPos(mousePos);

			auto le = [](const float2& lhs, const float2& rhs) {
				return lhs.x <= rhs.x and lhs.y <= rhs.y;
			};

			if (le(bottomLeft, mouseWorldPos) and le(mouseWorldPos, topRight)) {
				domain.addComponent<Button::Clicked>(entity);
			}
		}
	}
}