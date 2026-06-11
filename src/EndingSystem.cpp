#include <EndingSystem.h>
#include <archimedes/Scene.h>
#include <Utils.h>
#include <SlingshotSystem.h>
#include <ScoreSystem.h>
#include <DragonSystem.h>
#include <Button.h>
#include <LevelManager.h>

using namespace arch;

void EndingSystem::update() {
	auto&& domain = scene::SceneManager::get()->currentScene()->domain();
	auto&& levelState = domain.global<LevelState>();



	if (levelState != LevelState::playing) {
		for (auto&& [namedButton] : domain.view<NamedButton, Button::Clicked>().components()) {
			if (namedButton.name == "repeatButton") {
				LevelManager::reloadLevel();
			} else if (namedButton.name == "nextLevelButton") {
				LevelManager::nextLevel();
			}
		}

		return;
	}

	for (auto&& [body] : domain.view<b2BodyId>().components()) {
		if (isMoving(body)) {
			return;
		}
	}

	if (domain.components<Dragon>().base().count() != 0) {
		if (domain.components<Can>().base().count() == 0) {
			Logger::critical("You lost");
			levelState = LevelState::lost;
			auto windowJson = Json::parse(std::ifstream("settings/levelEnd.json"));
			parseStatusWindow(windowJson, "levelEndWindow", "loss");
			return;
		}
	} else {
		for (auto&& [can] : domain.view<Can>().components()) {
			ScoreSystem::add(can.points);
		}

		Logger::info("You won with {} points", ScoreSystem::get());
		levelState = LevelState::won;
		auto windowJson = Json::parse(std::ifstream("settings/levelEnd.json"));
		parseStatusWindow(windowJson, "levelEndWindow", "victory");
	}
}