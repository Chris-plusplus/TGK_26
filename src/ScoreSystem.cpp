#include <ScoreSystem.h>
#include <archimedes/Scene.h>
#include <archimedes/Text.h>
#include <Utils.h>
#include <glm/gtx/string_cast.hpp>
#include <sprite/Circle.h>
#include <archimedes/Font.h>

using namespace arch;

float ScoreSystem::get() {
	for (auto&& [score] : scene::SceneManager::get()->currentScene()->domain().view<ScoreData>().components()) {
		return score.value;
	}
	return 0.f;
}

void ScoreSystem::reset() {
	for (auto&& [score] : scene::SceneManager::get()->currentScene()->domain().view<ScoreData>().components()) {
		score.value = 0;
		score.update = true;
		break;
	}
}

void ScoreSystem::add(float value) {
	for (auto&& [score] : scene::SceneManager::get()->currentScene()->domain().view<ScoreData>().components()) {
		score.value += value;
		score.update = true;
		break;
	}
}

void ScoreSystem::setup() {
	auto score = scene::SceneManager::get()->currentScene()->newEntity();

	auto&& t = score.addComponent<scene::components::TransformComponent>();
	t.position = {};
	t.rotation = {0, 0, 0, 1};
	t.scale = {100, 100, 0};

	auto&& text = score.addComponent(
		text::TextComponent(
			std::u32string(U"dupa"),
			{defaultUniformBuffer()},
			font::FontDB::get()["Arial"]->regular().get()
		)
	);

	auto tmat = t.getTransformMatrix();
	auto tr = text.topRight(tmat);
	auto winSize = gfx::Renderer::current()->getWindow()->getSize();

	t.position = {0, winSize.y - tr.y, -0.9};

	score.addComponent<ScoreData>();
}

void ScoreSystem::update() {
	for (auto&& [score, t, text] : scene::SceneManager::get()->currentScene()->domain().view<ScoreData, scene::components::TransformComponent, text::TextComponent>().components()) {
		if (not score.update) {
			break;
		}
		score.update = false;

		text = text::TextComponent(
			std::u32string(std::from_range, std::format("Score: {:.2f}", score.value)),
			{defaultUniformBuffer()},
			font::FontDB::get()["Arial"]->regular().get()
		);
		auto tmat = t.getTransformMatrix();
		auto bl = text.bottomLeftAdjusted(tmat);
		auto tr = text.topRight(tmat);
		auto size = tr - bl;
		t.position.x = gfx::Renderer::current()->getWindow()->getSize().x - size.x;

		break;
	}
}