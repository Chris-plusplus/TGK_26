#pragma once

#include <archimedes/gfx/Texture.h>

using namespace arch;

Ref<gfx::Texture> makeCircleTexture(const u32 radius, Color color, i32 thickness = -1);
