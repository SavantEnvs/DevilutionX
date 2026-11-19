// TSV game-data tables (Source/data/file.cpp, parser.cpp, iterators.hpp): the format of every
// txtdata/*.tsv table, which mods can replace. Parses the header against a column map like the
// data loaders do, then walks every record and field through the typed field parsers.
// Byte 0 bit 0 selects parseHeader (column map) vs skipHeader. The rest is written to
// $TMPDIR/<scratch>/fuzz.tsv (overwritten for every input) and loaded through DataFile::load,
// the same asset lookup the game uses for txtdata files.
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <expected>
#include <memory>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

#include "data/file.hpp"
#include "data/iterators.hpp"
#include "utils/fixed_point.hpp"
#include "utils/paths.h"

using namespace devilution;

namespace {
enum class Column : uint8_t { Name, Level, Damage, Flags, Speed, LAST = Speed };

std::expected<Column, ColumnDefinition::Error> MapColumn(std::string_view label)
{
	if (label == "name") return Column::Name;
	if (label == "level") return Column::Level;
	if (label == "damage") return Column::Damage;
	if (label == "flags") return Column::Flags;
	if (label == "speed") return Column::Speed;
	return std::unexpected(ColumnDefinition::Error::UnknownColumn);
}
std::string g_tsvPath;
} // namespace

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	const char *tmp = std::getenv("TMPDIR");
	std::string dir = std::string((tmp != nullptr && *tmp != '\0') ? tmp : "/tmp") + "/devx_datafile_XXXXXX";
	if (mkdtemp(dir.data()) == nullptr)
		std::abort();
	paths::SetAssetsPath(dir + "/");
	g_tsvPath = dir + "/fuzz.tsv";
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
		return -1;
	const bool useColumnMap = (data[0] & 1) != 0;
	data += 1;
	size -= 1;

	FILE *f = std::fopen(g_tsvPath.c_str(), "wb");
	if (f == nullptr)
		std::abort();
	if (size != 0 && std::fwrite(data, 1, size, f) != size)
		std::abort();
	std::fclose(f);

	auto loaded = DataFile::load("fuzz.tsv");
	if (!loaded.has_value())
		return 0;
	DataFile &dataFile = *loaded;

	std::array<ColumnDefinition, static_cast<size_t>(Column::LAST) + 1> columns;
	if (useColumnMap) {
		auto header = dataFile.parseHeader<Column>(columns.data(), columns.data() + columns.size(), MapColumn);
		if (!header.has_value())
			return 0;
	} else {
		if (!dataFile.skipHeader().has_value())
			return 0;
	}
	(void)dataFile.numRecords();

	for (DataFileRecord record : dataFile) {
		unsigned col = 0;
		for (DataFileField field : record) {
			switch (col++ % 6) {
			case 0: {
				int32_t v;
				(void)field.parseInt(v);
				break;
			}
			case 1: {
				uint8_t v;
				(void)field.parseInt(v);
				break;
			}
			case 2: {
				Fixed26_6 v;
				(void)field.parseFixed6(v);
				break;
			}
			case 3: {
				bool v;
				(void)field.parseBool(v);
				break;
			}
			case 4: {
				int16_t arr[4];
				(void)field.parseIntArray(arr);
				break;
			}
			default:
				(void)field.value();
				break;
			}
		}
	}
	return 0;
}
