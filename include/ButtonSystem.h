#pragma once

#include <archimedes/Scene.h>

using namespace arch;

struct RepeatButton {
	static constexpr bool flagComponent = true;
};

struct ButtonSystem {
	static void update();
};