#pragma once

#include <archimedes/gfx/Texture.h>
#include <archimedes/asset/mesh/Mesh.h>
#include <Vertex.h>
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