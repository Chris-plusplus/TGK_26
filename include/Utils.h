#pragma once

#include <archimedes/gfx/Texture.h>
#include <archimedes/asset/mesh/Mesh.h>
#include <Vertex.h>
#include <box2d/box2d.h>
#include <Defaults.h>
#include <nlohmann/json.hpp>
#include <archimedes/Camera.h>
#include <archimedes/Scene.h>
#include <glm/gtx/string_cast.hpp>

using namespace arch;

using Json = nlohmann::json;

struct NamedButton {
	std::string name;
};

// creates mesh from vertices and indices
Ref<asset::mesh::Mesh> makeMesh(std::span<Vertex> vs, std::span<u32> is);

// creates texture from path relative to current working directory
Ref<gfx::Texture> loadTexture(std::string_view filename, gfx::TextureFilterMode filterMode = gfx::TextureFilterMode::nearest);

Ref<gfx::pipeline::Pipeline> makeCameraPipeline(
	const Ref<gfx::Texture>& texture,
	std::string vertexShader = "",
	std::string fragmentShader = ""
);

Ref<gfx::pipeline::Pipeline> makeScreenPipeline(
	const Ref<gfx::Texture>& texture,
	std::string vertexShader = "",
	std::string fragmentShader = ""
);

Camera& getCamera();
Window& getWindow();

struct StringViewHasher {
	using is_transparent = void; // Aktywuje mechanizm

	size_t operator()(std::string_view sv) const {
		return std::hash<std::string_view>{}(sv);
	}
	size_t operator()(const std::string& s) const {
		return std::hash<std::string>{}(s);
	}
	size_t operator()(const char* s) const {
		return std::hash<std::string_view>{}(s);
	}
};

template<class T>
using UnorderedMapString = std::unordered_map<std::string, T, StringViewHasher, std::equal_to<>>;

math::Quat angleToQuat(f32 angle);

void syncBodyToTransform(b2BodyId body, const scene::components::TransformComponent& t);

bool isMoving(b2BodyId body);

auto parseTexture(Json& json, std::string_view key) -> decltype(loadTexture(""));

float3 parseVec(Json& json, std::string_view key, bool log = true);

Entity parseButton(Json& json, std::string_view key);

Entity parseStatusWindow(Json& json, std::string_view which, std::string_view config);
