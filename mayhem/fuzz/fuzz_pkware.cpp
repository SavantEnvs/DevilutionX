// PKWARE DCL "explode" wrapper (Source/encrypt.cpp PkwareDecompress), used on network level
// deltas (msg.cpp DeltaImport) and compressed save blocks. Byte 0..1 pick the caller's output
// limit (maxBytes); the rest is the compressed stream, decompressed in place like the game does.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#include "encrypt.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 2)
		return -1;
	size_t maxBytes = (static_cast<size_t>(data[0]) | (static_cast<size_t>(data[1]) << 8)) + 1;
	data += 2;
	size -= 2;
	// The game's contract: inBuff holds at least maxBytes (the decompressed data is copied back
	// into it), and recvSize <= maxBytes.
	if (size > maxBytes)
		maxBytes = size;
	auto buf = std::make_unique<std::byte[]>(maxBytes);
	if (size != 0)
		std::memcpy(buf.get(), data, size);
	(void)devilution::PkwareDecompress(buf.get(), static_cast<uint32_t>(size), maxBytes);
	return 0;
}
