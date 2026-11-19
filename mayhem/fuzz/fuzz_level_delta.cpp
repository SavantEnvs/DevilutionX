// Multiplayer level deltas through the game-command dispatcher: ParseCmd (Source/msg.cpp), the
// function multi.cpp calls on every received message, fed CMD_DLEVEL / CMD_DLEVEL_JUNK chunks and
// the closing CMD_DLEVEL_END from a remote player. That runs OnLevelData -> DeltaImportData ->
// PkwareDecompress + DeltaImportJunk/Item/Objects/Monster/SpawnedMonsters, which parse the level
// state a joining player receives from the game host.
//
// ParseCmd is a command interpreter, so the harness allow-lists the three delta commands and
// builds their TCmdPlrInfoHdr framing the way DeltaExportData sends it (consecutive chunks with
// running offsets); the fuzzer controls the command choice, chunk sizes and every payload byte.
// Input: byte 0 bit 0 = CMD_DLEVEL vs CMD_DLEVEL_JUNK; then chunks of [len byte][len bytes].
//
// Kept across inputs on purpose: the loopback network provider and the read-only data tables.
// Reset per input: the delta store (delta_init) and the delta sender, so every input starts a
// fresh transfer.
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "headless_mode.hpp"
#include "msg.h"
#include "multi.h"
#include "player.h"
#include "storm/storm_net.hpp"
#include "tables/itemdat.h"
#include "tables/monstdat.h"
#include "tables/playerdat.hpp"
#include "tables/spelldat.h"
#include "utils/endian_swap.hpp"

using namespace devilution;

namespace {
constexpr uint8_t RemotePlayer = 1;

void AppendChunk(std::vector<uint8_t> &out, _cmd_id cmd, uint16_t offset, const uint8_t *data, uint16_t len)
{
	TCmdPlrInfoHdr hdr;
	hdr.bCmd = cmd;
	hdr.wOffset = Swap16LE(offset);
	hdr.wBytes = Swap16LE(len);
	const auto *h = reinterpret_cast<const uint8_t *>(&hdr);
	out.insert(out.end(), h, h + sizeof(hdr));
	out.insert(out.end(), data, data + len);
}
} // namespace

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	HeadlessMode = true;
	LoadSpellData();
	LoadPlayerDataFiles();
	LoadMonsterData();
	LoadItemData();
	Players.resize(MAX_PLRS);
	// A real game only has created characters in its slots (level >= 1); item regeneration during
	// delta validation reads the local player's level. (CreatePlayer itself needs the proprietary
	// cursor graphics, so only class and level are set.)
	for (Player &player : Players) {
		player._pClass = HeroClass::Warrior;
		player.setCharacterLevel(1);
	}
	MyPlayerId = 0;
	MyPlayer = &Players[0];
	gbIsMultiplayer = true;
	SNetInitializeProvider(SELCONN_LOOPBACK, nullptr);
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
		return -1;
	const _cmd_id cmd = (data[0] & 1) != 0 ? CMD_DLEVEL_JUNK : CMD_DLEVEL;
	data += 1;
	size -= 1;

	delta_init();
	gbDeltaSender = MAX_PLRS;

	std::vector<uint8_t> stream;
	uint32_t offset = 0;
	size_t pos = 0;
	while (pos < size) {
		size_t len = data[pos++];
		if (len > size - pos)
			len = size - pos;
		if (offset + len > 0xFFFF)
			break;
		AppendChunk(stream, cmd, static_cast<uint16_t>(offset), data + pos, static_cast<uint16_t>(len));
		offset += static_cast<uint32_t>(len);
		pos += len;
	}
	AppendChunk(stream, CMD_DLEVEL_END, 0, nullptr, 0);

	// The receive loop of multi.cpp (ProcessGameMessagePackets): parse until the buffer is used up.
	size_t at = 0;
	while (at < stream.size()) {
		const size_t used = ParseCmd(RemotePlayer, reinterpret_cast<const TCmd *>(&stream[at]), stream.size() - at);
		if (used == 0)
			break;
		at += used;
	}
	return 0;
}
