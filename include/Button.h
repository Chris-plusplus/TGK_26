#pragma once

#include <archimedes/Scene.h>
#include <archimedes/Input.h>
#include <archimedes/Camera.h>
#include <archimedes/Mmath.h>
#include <array>

using namespace arch;

struct Button {
	struct Clicked {
		static constexpr bool flagComponent = true;
	};

	Camera* camera = nullptr;
	std::array<u32, 14> keycodes{};
};