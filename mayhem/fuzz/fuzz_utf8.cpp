// UTF-8 helpers (Source/utils/utf8.*) applied to player names and chat text from the network:
// code-point decoding, truncating copies into fixed-size name buffers, re-encoding.
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "utils/utf8.hpp"

using namespace devilution;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	std::string_view input { reinterpret_cast<const char *>(data), size };
	std::string reencoded;
	std::string_view rest = input;
	while (!rest.empty()) {
		const char32_t cp = ConsumeFirstUtf8CodePoint(&rest);
		AppendUtf8(cp, reencoded);
	}
	(void)FindLastUtf8Symbols(input);
	char name[32];
	CopyUtf8(name, input, sizeof(name));
	char small[4];
	CopyUtf8(small, input, sizeof(small));
	return 0;
}
