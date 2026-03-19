#include <CellSystem.h>
#include <Cell.h>
#include <CellMember.h>
#include <Grid.h>
#include <SimConfig.h>
#include <algorithm>
#include <random>
#include <Defaults.h>
#include <States.h>
#include <Textures.h>

void CellSystem::initCities() {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();

	for (auto&& [i, grid] : std::views::enumerate(simConfig.cities)) {
		auto gridE = scene.newEntity();
		auto&& gridC = gridE.addComponent(
			Grid{
				.cells = {},
				.bottomLeft = {grid.left, grid.bottom},
				.topRight = {grid.left + grid.length, grid.bottom + grid.length},
				.size = (u32)std::ceil((grid.length - grid.left) / simConfig.infectionRadius),
				.i = (u32)i
			}
		);
		ARCH_FORCE_ASSERT(gridC.bottomLeft.x - gridC.topRight.x == gridC.bottomLeft.y - gridC.topRight.y, "grid must be a rectangle");

		for (u32 y = 0; y != gridC.size; ++y) {
			for (u32 x = 0; x != gridC.size; ++x) {
				auto cellE = gridE.addChild();
				gridC.cells.push_back(&cellE.addComponent<Cell>());
			}
		}
		auto cellAt = [&cells = gridC.cells, size = gridC.size](const u32 x, const u32 y) -> Cell* {
			if (y >= size or x >= size) {
				return nullptr;
			}
			return cells[y * size + x];
		};
		for (u32 y = 0; y != gridC.size; ++y) {
			for (u32 x = 0; x != gridC.size; ++x) {
				auto&& cell = *cellAt(x, y);

				for (i32 dy = -1; dy <= 1; ++dy) {
					for (i32 dx = -1; dx <= 1; ++dx) {
						if (dx == 0 and dy == 0) continue;

						auto neighbor = cellAt(x + dx, y + dy);
						if (neighbor) {
							cell.neighbors.push_back(neighbor);
						}
					}
				}
			}
		}
	}
}

void CellSystem::initPopulation() {
	auto scene = scene::SceneManager::get()->currentScene();
	auto&& domain = scene->domain();
	auto rng = std::minstd_rand(std::random_device{}());
	auto mesh = defaultMesh();
	auto&& textures = domain.global<Textures>();

	std::vector<ecs::Entity> allActors;
	allActors.reserve(simConfig.cities.size() * simConfig.populationPerCity);

	for (auto&& [grid] : domain.view<Grid>().components()) {
		auto&& city = simConfig.cities[grid.i];
		auto distX = std::uniform_real_distribution<f32>(city.left, city.left + city.length);
		auto distY = std::uniform_real_distribution<f32>(city.bottom, city.bottom + city.length);

		for (u32 i = 0; i != simConfig.populationPerCity; ++i) {
			auto actor = scene->newEntity();
			allActors.push_back(actor);
			actor.addComponent(
				scene::components::TransformComponent{
					.position = {distX(rng), distY(rng), 0},
					.rotation = {0, 0, 0, -1},
					.scale = {simConfig.infectionRadius * 2, simConfig.infectionRadius * 2, 0}
				}
			);
			actor.addComponent(
				scene::components::MeshComponent{
					.mesh = mesh,
					.pipeline = textures.susceptible
				}
			);
			actor.addComponent<CellMember>(&grid);
			actor.addComponent<Susceptible>();
		}
	}

	std::ranges::shuffle(allActors, rng);
	for (auto&& infected : allActors | std::views::take(simConfig.initialInfected)) {
		domain.removeComponent<Susceptible>(infected);
		domain.addComponent<Infected>(infected);
		domain.getComponent<scene::components::MeshComponent>(infected).pipeline = textures.infected;
	}
}

void CellSystem::init() {
	initCities();
	initPopulation();
	rebuild();
}

void CellSystem::rebuild() {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();

	for (auto&& [cell] : domain.view<Cell>().components()) {
		cell.actors.clear();
	}

	const auto mul = 1.f / simConfig.infectionRadius;
	for (auto&& [entity, cellMember, t] : domain.view<CellMember, scene::components::TransformComponent>().all()) {
		uint2 pos = uint2(((float2)t.position - cellMember.grid->bottomLeft) * mul);
		pos.x = std::min<int>(pos.x, cellMember.grid->size - 1);
		pos.y = std::min<int>(pos.y, cellMember.grid->size - 1);

		auto&& cell = *cellMember.grid->cells[pos.y * cellMember.grid->size + pos.x];
		cell.actors.emplace_back(entity, t.position);
		cellMember.cell = &cell;
	}
}

std::vector<ecs::Entity> CellSystem::findNeighbors(ecs::Entity actor) {
	std::vector<ecs::Entity> result;

	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();

	auto const& member = domain.getComponent<CellMember>(actor);
	auto const& t = domain.getComponent<scene::components::TransformComponent>(actor);
	const auto rSqr = simConfig.infectionRadius * simConfig.infectionRadius;
	const auto pos = (float2)t.position;

	u32 upperBound = member.cell->actors.size() - 1;
	for (auto const& neighbor : member.cell->neighbors) {
		upperBound += neighbor->actors.size();
	}
	result.reserve(upperBound);

	for (auto const& [neighbor, neighborT] : member.cell->actors) {
		if (neighbor == actor) {
			continue;
		}

		auto dstXSqr = pos.x - neighborT.x;
		auto dstYSqr = pos.y - neighborT.y;
		dstXSqr *= dstXSqr;
		dstYSqr *= dstYSqr;

		if (dstXSqr + dstYSqr - rSqr < 0) {
			result.push_back(neighbor);
		}
	}
	for (auto const& cell : member.cell->neighbors) {
		for (auto const& [neighbor, neighborT] : cell->actors) {

			auto dstXSqr = pos.x - neighborT.x;
			auto dstYSqr = pos.y - neighborT.y;
			dstXSqr *= dstXSqr;
			dstYSqr *= dstYSqr;

			if (dstXSqr + dstYSqr - rSqr < 0) {
				result.push_back(neighbor);
			}
		}
	}

	return result;
}