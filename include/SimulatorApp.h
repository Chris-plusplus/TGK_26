#pragma once

#include <archimedes/Application.h>
#include <archimedes/physics/System.h>
#include <archimedes/Ref.h>

using namespace arch;

// VulkanVs game
class SimulatorApp: public Application {
public:
	SimulatorApp() = default;

	void init() override;
	void update() override;

private:
	Ref<physics::System> _physicsSystem;
};
