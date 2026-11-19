// PCX image -> CLX sprite conversion (Source/utils/pcx_to_clx.cpp), the loader for every
// PCX asset (game data and mods), then renders each converted frame like the game would.
// Input: byte 0 = frame count / negative frame height, byte 1 = transparent-colour selector,
// rest = the PCX file.
#include <cstddef>
#include <cstdint>
#include <optional>

#include <SDL.h>

#include "engine/assets.hpp"
#include "engine/clx_sprite.hpp"
#include "engine/render/clx_render.hpp"
#include "engine/surface.hpp"
#include "utils/pcx_to_clx.hpp"
#include "utils/sdl_compat.h"
#include "utils/sdl_wrap.h"

using namespace devilution;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 2)
		return -1;
	// numFramesOrFrameHeight: "a positive value with the number of frames, or the frame
	// height as a negative value" (pcx_to_clx.hpp); 0 is outside that contract.
	int numFramesOrFrameHeight = static_cast<int8_t>(data[0]);
	if (numFramesOrFrameHeight == 0)
		numFramesOrFrameHeight = 1;
	std::optional<uint8_t> transparent;
	if ((data[1] & 1) != 0)
		transparent = data[1];
	data += 2;
	size -= 2;

	AssetHandle handle { SDL_IOFromConstMem(data, static_cast<int>(size)) };
	SDL_Color palette[256];
	auto result = PcxToClx(handle, size, numFramesOrFrameHeight, transparent, palette);
	if (!result.has_value())
		return 0;

	const ClxSpriteList list = *result;
	for (const ClxSprite sprite : list) {
		const int w = sprite.width();
		const int h = sprite.height();
		if (w <= 0 || h <= 0 || static_cast<int64_t>(w) * h > (1 << 22))
			continue;
		SDLSurfaceUniquePtr surface = SDLWrap::CreateRGBSurfaceWithFormat(0, w, h, 8, SDL_PIXELFORMAT_INDEX8);
		const Surface out { surface.get() };
		ClxDraw(out, { 0, out.h() - 1 }, sprite);
	}
	return 0;
}
