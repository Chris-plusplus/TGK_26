#include <Utils.h>
#include <archimedes/gfx/Renderer.h>
#include <stb_image.h>
#include <execution>

Ref<gfx::Texture> loadTexture(std::string_view filename) {
	auto renderer = gfx::Renderer::getCurrent();

	int ignored;
	int width;
	int height;
	stbi_set_flip_vertically_on_load(false);
	// load texture from file
	u8* loadedTextureData = stbi_load(filename.data(), &width, &height, &ignored, STBI_rgb_alpha);

	if (!loadedTextureData) {
		Logger::error("{}: file '{}' not found", __func__, filename);
		throw std::runtime_error(std::format("{}: file '{}' not found", __func__, filename));
	}

	auto textureData = std::vector<Color>(width * height);
	// copy the texture and normalize RBGA
	std::for_each(
		std::execution::par_unseq,
		textureData.begin(),
		textureData.end(),
		[begin = textureData.data(), &loadedTextureData](Color& color) {
		const auto i = &color - begin;
		color.r = loadedTextureData[4 * i + 0] / 255.f;
		color.g = loadedTextureData[4 * i + 1] / 255.f;
		color.b = loadedTextureData[4 * i + 2] / 255.f;
		color.a = loadedTextureData[4 * i + 3] / 255.f;
	}
	);
	std::ranges::reverse(textureData);
	stbi_image_free(loadedTextureData);

	return renderer->getTextureManager()->createTexture2D(width, height, textureData.data());
}

Ref<asset::mesh::Mesh> makeMesh(std::span<Vertex> vs, std::span<u32> is) {
	auto bufferManager = gfx::Renderer::getCurrent()->getBufferManager();
	return asset::mesh::Mesh::create(
		bufferManager->createVertexBuffer(vs),
		bufferManager->createIndexBuffer(is)
	);
}

Ref<gfx::pipeline::Pipeline> makePipeline(
	const Ref<gfx::Texture>& texture,
	std::string vertexShader,
	std::string fragmentShader
) {
	if (vertexShader.empty()) {
		vertexShader = defaultVertexShader();
	}
	if (fragmentShader.empty()) {
		fragmentShader = defaultFragmentShader();
	}

	return gfx::Renderer::getCurrent()->getPipelineManager()->create(
		gfx::pipeline::Pipeline::Desc{
			.vertexShaderPath = std::move(vertexShader),
			.fragmentShaderPath = std::move(fragmentShader),
			.textures = {texture},
			.buffers = {defaultUniformBuffer()}
		}
	);
}

math::Quat angleToQuat(f32 angle) {
	return glm::angleAxis(angle, float3{0.f, 0.f, 1.f});
}

void syncBodyToTransform(b2BodyId body, const scene::components::TransformComponent& t) {
	b2Body_SetTransform(body, b2Vec2{t.position.x * scale, t.position.y * scale}, b2Body_GetRotation(body));
}

bool isMoving(b2BodyId body) {
	constexpr auto eps = std::numeric_limits<float>::epsilon();
	auto vel = b2Body_GetLinearVelocity(body);
	return glm::abs(vel.x) >= eps or glm::abs(vel.y) >= eps or glm::abs(b2Body_GetAngularVelocity(body)) >= eps;
}