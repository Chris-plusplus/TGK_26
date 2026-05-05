#pragma once

#include <box2d/base.h>
#include <box2d/box2d.h>
#include <box2d/collision.h>
#include <box2d/id.h>
#include <box2d/math_functions.h>
#include <box2d/types.h>

struct b2WorldWrapper {
	b2WorldId id = b2_nullWorldId;
	float scale = 1;
	float deltaTime = 0;

	~b2WorldWrapper() {
		b2DestroyWorld(id);
		id = b2_nullWorldId;
	}
};