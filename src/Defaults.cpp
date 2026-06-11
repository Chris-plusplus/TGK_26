#include <Defaults.h>
#include <EngineConfig.h>
#include <archimedes/Camera.h>
#include <archimedes/Scene.h>

UBO ubo{glm::ortho(0.f, (float)windowWidth, 0.f, (float)windowHeight)};

float2 cameraDelta{};

extern Ref<gfx::buffer::Buffer> ubobuf{};

WindowFixedCamera::WindowFixedCamera(): Camera() {
	auto monitor = *Monitor::get();
	setPos(monitor.originalSize() / 2);
	setExtents(monitor.originalSize() / 2);
}

// default uniform buffer with orthographic projection matrix
Ref<gfx::buffer::Buffer> cameraUniformBuffer() {
	return scene::SceneManager::get()->currentScene()->domain().global<Camera>().buffer();
}
Ref<gfx::buffer::Buffer> screenUniformBuffer() {
	return scene::SceneManager::get()->currentScene()->domain().global<WindowFixedCamera>().buffer();
}

// default vertices for particles (makes rotating easier)
std::vector<Vertex>& defaultVertices() {
	static std::vector<Vertex> value{
		{{0.5f, -0.5f, 0.f}, {0.f, 0.f}},
		{{-0.5f, -0.5f, 0.f}, {1.f, 0.f}},
		{{0.5f, 0.5f, 0.f}, {0.f, 1.f}},
		{{-0.5f, 0.5f, 0.f}, {1.f, 1.f}},
	};
	return value;
}

// default indices
std::vector<u32>& defaultIndices() {
	static std::vector<u32> value{0, 1, 2, 2, 1, 3};
	return value;
}

Ref<asset::mesh::Mesh> defaultMesh() {
	auto bufferManager = gfx::Renderer::getCurrent()->getBufferManager();
	return asset::mesh::Mesh::create(
		bufferManager->createVertexBuffer(std::span(defaultVertices())),
		bufferManager->createIndexBuffer(defaultIndices())
	);
}

std::string defaultVertexShader() {
	return "shaders/vertex_default.glsl";
}

std::string defaultFragmentShader() {
	return "shaders/fragment_default.glsl";
}

std::string defaultFontFragmentShader() {
	return "shaders/text/fragment_atlas.glsl";
}

float scale = 1.f / 128.f;