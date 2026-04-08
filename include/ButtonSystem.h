#pragma once

#include <archimedes/Scene.h>

using namespace arch;

struct ButtonSystem {
	template<class T>
	void makeButton(Entity entity);

	void makeButton(Entity entity);

	void update(Scene& scene);
};