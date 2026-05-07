#pragma once

#include <archimedes/gfx/Texture.h>
#include <archimedes/asset/mesh/Mesh.h>
#include <Vertex.h>
#include <box2d/box2d.h>
#include <Defaults.h>

using namespace arch;

// creates mesh from vertices and indices
Ref<asset::mesh::Mesh> makeMesh(std::span<Vertex> vs, std::span<u32> is);

// creates texture from path relative to current working directory
Ref<gfx::Texture> loadTexture(std::string_view filename);

Ref<gfx::pipeline::Pipeline> makePipeline(
	const Ref<gfx::Texture>& texture,
	std::string vertexShader = "",
	std::string fragmentShader = ""
);

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