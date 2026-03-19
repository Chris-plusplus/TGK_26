#include <sprite/Circle.h>
#include <archimedes/gfx/Renderer.h>

Ref<gfx::Texture> makeCircleTexture(const u32 radius, Color color, i32 thickness) {
	if (thickness == -1) {
		thickness = radius;
	}
	auto size = radius * 2;
	auto textureData = std::vector<Color>(size * size);
	auto currPx = textureData.data();
	auto radiusSqr = radius * radius;
	auto hollowRadiusSqr = (radius - thickness) * (radius - thickness);

	for (u32 j = 0; j != size; ++j) {
		float y = j + 0.5f - radius;
		float ySqr = y * y;
		for (u32 i = 0; i != size; ++i) {
			float x = i + 0.5f - radius;
			auto dist = x * x + ySqr;

			bool inside = dist <= radiusSqr and dist > hollowRadiusSqr;
			*currPx++ = (f32)inside * color;
		}
	}

	return gfx::Renderer::getCurrent()->getTextureManager()->createTexture2D(size, size, textureData.data());
}