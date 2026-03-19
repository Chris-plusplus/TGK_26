#include <SimulatorApp.h>
#include <archimedes/Scene.h>
#include <Defaults.h>
#include <EngineConfig.h>
#include <Utils.h>
#include <sprite/Circle.h>
#include <Cell.h>
#include <CellSystem.h>
#include <CellMember.h>
#include <Grid.h>
#include <SimConfig.h>
#include <random>
#include <numbers>
#include <Textures.h>
#include <States.h>

struct MouseControlled {
	static constexpr bool flagComponent = true;
};

struct Movement {
	f32 angle;
	f32 deltaAngle;
};

void SimulatorApp::init() {
	// make scene
	Ref<Scene> scene = createRef<Scene>();
	scene::SceneManager::get()->changeScene(scene);

	auto&& textures = scene->domain().global<Textures>();
	auto radius = std::max(simConfig.infectionRadius, 1.f);
	textures.susceptible = makePipeline(makeCircleTexture(radius, {0.f, 1.f, 0.f, 1.f}));
	textures.infected = makePipeline(makeCircleTexture(radius, {1.f, 0.f, 0.f, 1.f}));
	textures.removed = makePipeline(makeCircleTexture(radius, {0.9f, 0.9f, 0.9f, 1.f}));

	CellSystem::init();

	auto rng = std::mt19937(std::random_device{}());
	auto dist = std::uniform_real_distribution(0.f, 2.f * std::numbers::pi_v<f32>);
	for (auto entity : scene->domain().view<CellMember>()) {
		scene->domain().addComponent<Movement>(entity, dist(rng), dist(rng));
	}
}

void SimulatorApp::update() {
	//std::this_thread::sleep_for(std::chrono::milliseconds(16));

	Ref<Scene> scene = scene::SceneManager::get()->currentScene();
	auto&& domain = scene->domain();

	CellSystem::rebuild();

	auto rng = std::minstd_rand(std::random_device{}());
	auto infectionDist = std::uniform_real_distribution(0.f, 1.f);
	auto toInfect = std::vector<ecs::Entity>();
	auto toRemove = std::vector<ecs::Entity>();
	//auto start = std::chrono::high_resolution_clock::now();
	for (auto&& [e, member, infected] : domain.view<CellMember, Infected>().all()) {
		for (auto neighbor : CellSystem::findNeighbors(e)) {
			if (domain.hasComponent<Susceptible>(neighbor) && infectionDist(rng) < simConfig.infectionChance) {
				toInfect.push_back(neighbor);
			}
		}

		if (++infected.timer >= simConfig.timeToRemove) {
			toRemove.push_back(e);
		}
	}
	auto&& textures = domain.global<Textures>();
	for (auto&& neighbor : toInfect) {
		domain.removeComponent<Susceptible>(neighbor);
		domain.addComponent<Infected>(neighbor);
		domain.getComponent<scene::components::MeshComponent>(neighbor).pipeline = textures.infected;
	}
	for (auto&& entity : toRemove) {
		domain.removeComponent<Infected>(entity);
		domain.getComponent<scene::components::MeshComponent>(entity).pipeline = textures.removed;
		domain.kill(entity);
	}
	//auto end = std::chrono::high_resolution_clock::now();
	//Logger::debug("{}", std::chrono::duration_cast<std::chrono::milliseconds>(end - start));

	auto jerkDist = std::uniform_real_distribution(-simConfig.maxAngleJerk, simConfig.maxAngleJerk);
	for (auto&& [movement, t, member] : domain.view<Movement, scene::components::TransformComponent, CellMember>().components()) {
		movement.angle += movement.deltaAngle;
		movement.deltaAngle = std::clamp(movement.deltaAngle * 0.9f + jerkDist(rng), -simConfig.maxAngleChange, simConfig.maxAngleChange);

		t.position.x += std::cos(movement.angle) * simConfig.actorSpeed;
		t.position.y += std::sin(movement.angle) * simConfig.actorSpeed;

		if (
			t.position.x < member.grid->bottomLeft.x or
			t.position.x > member.grid->topRight.x
			) {
			movement.angle = std::numbers::pi_v<f32> -movement.angle;
		}
		if (
			t.position.y < member.grid->bottomLeft.y or
			t.position.y > member.grid->topRight.y
			) {
			movement.angle = -movement.angle;
		}

		t.position.x = std::clamp(t.position.x, member.grid->bottomLeft.x, member.grid->topRight.x);
		t.position.y = std::clamp(t.position.y, member.grid->bottomLeft.y, member.grid->topRight.y);
	}
}
