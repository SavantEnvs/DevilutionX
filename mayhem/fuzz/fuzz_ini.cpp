// INI parser (Source/utils/ini.cpp) used for diablo.ini and mod configuration, then the typed
// accessors and a serialize round-trip. getBool/getInt/getFloat are left out on purpose: they
// app_fatal() by design on a value of the wrong type (ini.cpp), which is not a bug.
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

#include "utils/ini.hpp"

using namespace devilution;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	std::string_view buffer { reinterpret_cast<const char *>(data), size };
	auto result = Ini::parse(buffer);
	if (!result.has_value())
		return 0;
	Ini &ini = *result;
	for (std::string_view section : { "", "Diablo", "Hellfire", "Graphics", "Audio", "Network", "Language", "Mods", "Controller" }) {
		for (const std::string &key : ini.getKeys(section)) {
			(void)ini.get(section, key);
			(void)ini.getString(section, key);
			char buf[32];
			ini.getUtf8Buf(section, key, buf, sizeof(buf));
		}
	}
	ini.set("Diablo", "Fuzz", 1);
	ini.set("Graphics", "Fuzz", std::string_view("value"));
	std::string out = ini.serialize();
	auto again = Ini::parse(out);
	(void)again;
	return 0;
}
