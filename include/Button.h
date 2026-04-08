#pragma once

#include <archimedes/Scene.h>
#include <variant>

using namespace arch;

struct Button {
	enum Type {
		circular,
		rectangular
	} type;

	struct Clicked {
		static constexpr bool flagComponent = true;
	};
};