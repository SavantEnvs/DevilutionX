// Save-file block decryption + SHA-1 signature check (Source/codec.cpp), applied to every
// entry of a .sv/.hsv save before it is parsed. Uses the single-player save password.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include "codec.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	auto buf = std::make_unique<std::byte[]>(size);
	if (size != 0)
		std::memcpy(buf.get(), data, size);
	(void)devilution::codec_decode(buf.get(), size, "xrgyrkj1");
	return 0;
}
