#include <archimedes/Engine.h>
#include <SimulatorApp.h>
#include <EngineConfig.h>
#include <sprite/Circle.h>

int main() {
	arch::Logger::init(arch::LogLevel::debug);

	arch::Unique<arch::Application> application = arch::createUnique<SimulatorApp>();

	auto engineConfig = arch::EngineConfig{
		.windowWidth = (int)windowWidth,
		.windowHeight = (int)windowHeight,
		.windowTitle = "Epidemic Simulator",
		.backgroundColor = arch::Color(1, 1, 1, 1),
		.renderingApi = arch::gfx::RenderingAPI::Nvrhi_VK
	};

	auto engine = arch::Engine(engineConfig, application);
	engine.start();
}