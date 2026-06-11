#include <Utils.h>
#include <archimedes/gfx/Renderer.h>
#include <archimedes/Scene.h>
#include <stb_image.h>
#include <execution>
#include <Button.h>
#include <archimedes/Text.h>
#include <archimedes/Font.h>
#include <ScoreSystem.h>
#include <format>
#include <LevelManager.h>

using namespace arch;

Ref<gfx::Texture> loadTexture(std::string_view filename, gfx::TextureFilterMode filterMode) {
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

	return renderer->getTextureManager()->createTexture2D(width, height, textureData.data(), gfx::GraphicsFormat::rgba32f,
		gfx::TextureWrapMode::repeat,
		filterMode);
}

Ref<asset::mesh::Mesh> makeMesh(std::span<Vertex> vs, std::span<u32> is) {
	auto bufferManager = gfx::Renderer::getCurrent()->getBufferManager();
	return asset::mesh::Mesh::create(
		bufferManager->createVertexBuffer(vs),
		bufferManager->createIndexBuffer(is)
	);
}

Ref<gfx::pipeline::Pipeline> makeCameraPipeline(
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
			.buffers = {cameraUniformBuffer()}
		}
	);
}

Ref<gfx::pipeline::Pipeline> makeScreenPipeline(
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
			.buffers = {screenUniformBuffer()}
		}
	);
}

Camera& getCamera() {
	return scene::SceneManager::get()->currentScene()->domain().global<Camera>();
}

Window& getWindow() {
	return *gfx::Renderer::getCurrent()->getWindow();
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

auto parseTexture(Json& json, std::string_view key) -> decltype(loadTexture("")) {
	if (json.is_null() or key.empty()) {
		return nullptr;
	}

	auto&& textureData = json[key];
	if (textureData.is_null()) {
		return nullptr;
	} else if (textureData.is_array()) {
		Color color = {};
		color.r = textureData[0];
		color.g = textureData[1];
		color.b = textureData[2];
		color.a = textureData[3];

		return gfx::Renderer::current()->getTextureManager()->createTexture2D(1, 1, &color);
	} else if (textureData.is_string()) {
		return loadTexture(textureData);
	} else {
		auto texturePath = textureData["path"].get<std::string>();
		auto filterMode = textureData.value("filterMode", "nearest");

		return loadTexture(texturePath, [&] -> gfx::TextureFilterMode {
			if (filterMode == "nearest") {
				return gfx::TextureFilterMode::nearest;
			} else if (filterMode == "linear") {
				return gfx::TextureFilterMode::linear;
			} else if (filterMode == "nearestMipmapNearest") {
				return gfx::TextureFilterMode::nearestMipmapNearest;
			} else if (filterMode == "linearMipmapNearest") {
				return gfx::TextureFilterMode::linearMipmapNearest;
			} else if (filterMode == "nearestMipmapLinear") {
				return gfx::TextureFilterMode::nearestMipmapLinear;
			} else if (filterMode == "linearMipmapLinear") {
				return gfx::TextureFilterMode::linearMipmapLinear;
			} else {
				Logger::error("'{}': invalid filterMode: '{}', using linear", texturePath, filterMode);
				return gfx::TextureFilterMode::linear;
			}
		}());
	}
}

float3 parseVec(Json& json, std::string_view key, bool log) {
	float3 result = {};

	if (json.is_null()) {
		if (log) Logger::error("position's parent json was null, returning zero");
		return result;
	}

	auto positionJsonFound = json.find(key);
	if (positionJsonFound == json.end()) {
		if (log) Logger::warn("position under '{}' key was not found, returning zero", key);
		return result;
	} else {
		auto& positionJson = *positionJsonFound;
		if (not positionJson.is_array() or positionJson.size() < 2) {
			if (log) Logger::error("position under '{}' key was invalid, returning zero", key);
			return result;
		} else {
			if (positionJson.size() > 1) {
				result.x = positionJson[0];
				result.y = positionJson[1];
			}
			if (positionJson.size() > 2) {
				result.z = positionJson[2];
			}
		}
	}
	return result;
}

Entity parseButton(Json& json, std::string_view key) {
	auto buttonJsonFound = json.find(key);
	if (buttonJsonFound == json.end()) {
		Logger::error("button '{}' was not found", key);
		return {};
	}
	auto& buttonJson = *buttonJsonFound;

	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto button = scene.newEntity();

	float3 scale = {0, 0, 1};
	scale.x = buttonJson["size"][0];
	scale.y = buttonJson["size"][1];

	auto&& t = button.addComponent(
		scene::components::TransformComponent{
			.position = parseVec(buttonJson, "position"),
			.rotation = {},
			.scale = scale
		}
	);

	button.addComponent(
		scene::components::MeshComponent{
			.mesh = defaultMesh(),
			.pipeline = makeScreenPipeline(parseTexture(buttonJson, "texture"))
		}
	);

	auto&& buttonC = button.addComponent<Button>();
	buttonC.camera = &scene.domain().global<WindowFixedCamera>();

	auto&& keycodes = buttonJson["keycode"];
	if (keycodes.is_array()) {
		for (auto&& [i, keycode] : keycodes | std::views::enumerate) {
			if (i >= 14) break;

			buttonC.keycodes[i] = keycode.get<u32>();
		}
	}

	return button;
}

Entity parseStatusWindow(Json& json, std::string_view which, std::string_view config) {
	auto&& scene = *scene::SceneManager::get()->currentScene();
	auto&& domain = scene.domain();

	auto&& windowJson = json[which];

	auto windowPosition = parseVec(windowJson, "position");
	auto windowSize = parseVec(windowJson, "size");
	auto windowTexture = parseTexture(windowJson, "texture");
	auto windowPipeline = makeScreenPipeline(windowTexture);

	auto configJsonFound = windowJson.find(config);
	if (configJsonFound == windowJson.end()) {
		Logger::error("Config '{}' was not found", config);
		return {};
	}
	auto& configJson = *configJsonFound;

	Entity window = scene.newEntity();
	window.addComponent<LevelEntity>();
	auto&& windowT = window.addComponent(
		scene::components::TransformComponent{
			.position = windowPosition,
			.rotation = angleToQuat(0),
			.scale = float3(windowSize.x, windowSize.y, 1)
		}
	);
	window.addComponent(
		scene::components::MeshComponent{
			.mesh = defaultMesh(),
			.pipeline = windowPipeline
		}
	);
	auto&& windowCamera = domain.global<WindowFixedCamera>();

	{ // Message
		auto& messageJson = configJson["text"];
		auto messagePosition = parseVec(messageJson, "position");
		auto&& face = *font::FontDB::get()[messageJson["font"].get<std::string>()]->regular();
		auto sizePx = messageJson["sizePx"].get<float>();
		auto value = messageJson["value"].get<std::string>();

		auto message = window.addChild();
		message.addComponent<LevelEntity>();
		auto messageXY = messagePosition + windowPosition;
		auto&& messageT = message.addComponent(
			scene::components::TransformComponent{
				.position = float3{messageXY.x, messageXY.y, messagePosition.z},
				.rotation = {0, 0, 0, 1},
				.scale = float3(sizePx, sizePx, 1)
			}
		);

		std::u32string str = text::convertTo<char32_t>(std::string_view(value));
		std::vector<Ref<gfx::buffer::Buffer>> buffers = {screenUniformBuffer()};
		font::Face& f = face;
		message.addComponent<text::TextComponent>(
			str, buffers, face
		);
	}

	{ // Score
		auto& messageJson = configJson["scoreText"];
		auto messagePosition = parseVec(messageJson, "position");
		auto&& face = *font::FontDB::get()[messageJson["font"].get<std::string>()]->regular();
		auto sizePx = messageJson["sizePx"].get<float>();
		auto valueFormat = messageJson["value"].get<std::string>();

		auto score = ScoreSystem::get();
		auto value = std::vformat(valueFormat, std::make_format_args(score));

		auto message = window.addChild();
		message.addComponent<LevelEntity>();
		auto messageXY = messagePosition + windowPosition;
		auto&& messageT = message.addComponent(
			scene::components::TransformComponent{
				.position = float3{messageXY.x, messageXY.y, messagePosition.z},
				.rotation = {0, 0, 0, 1},
				.scale = float3(sizePx, sizePx, 1)
			}
		);
		std::u32string str = text::convertTo<char32_t>(std::string_view(value));
		std::vector<Ref<gfx::buffer::Buffer>> buffers = {screenUniformBuffer()};
		font::Face& f = face;
		message.addComponent<text::TextComponent>(
			str, buffers, face
		);
	}

	{ // Buttons
		for (auto&& buttonJson : configJson["buttons"]) {
			auto first = buttonJson.items().begin().key();
			auto button = parseButton(buttonJson, std::string_view(first));
			button.addComponent<LevelEntity>();
			button.addComponent<NamedButton>(first);

			auto&& buttonT = button.getComponent<scene::components::TransformComponent>();
			buttonT.position.x += windowPosition.x;
			buttonT.position.y += windowPosition.y;
		}
	}
}