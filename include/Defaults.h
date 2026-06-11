#pragma once

#include <Vertex.h>
#include <archimedes/gfx/Renderer.h>
#include <archimedes/scene/Components.h>
#include <archimedes/Camera.h>

using namespace arch;

class WindowFixedCamera: public Camera {
public:
	WindowFixedCamera();
};

// returns default uniform buffer, containing orthographic projection matrix
Ref<gfx::buffer::Buffer> cameraUniformBuffer();
Ref<gfx::buffer::Buffer> screenUniformBuffer();

// returns default vertices, for displaying sprites
std::vector<Vertex>& defaultVertices();

// returns default indices, for displaying sprites
std::vector<u32>& defaultIndices();

// returns Z axis for rotations
constexpr float3 zAxis() {
	return float3{0, 0, 1};
}

Ref<asset::mesh::Mesh> defaultMesh();

std::string defaultVertexShader();

std::string defaultFragmentShader();

std::string defaultFontFragmentShader();

struct UBO {
	Mat4x4 projection;
};

extern UBO ubo;

extern float2 cameraDelta;

extern Ref<gfx::buffer::Buffer> ubobuf;

extern float scale;