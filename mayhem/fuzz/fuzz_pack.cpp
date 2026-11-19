// Packed player/item records (Source/pack.cpp): the structs stored in a hero save ("hero" entry
// of a .sv file: PlayerPack, ItemPack) and sent over the network (PlayerNetPack in
// CMD_SEND_PLRINFO, ItemNetPack in item commands). Byte 0 selects the entry point and the
// Diablo/Hellfire mode, the rest is copied into the packed struct (zero-padded when short).
//
// Init loads the game-data tables the unpackers consult (spells, classes, monsters, items) from
// the assets/ directory next to the binary, like upstream's pack_test. Per input the player
// slots are reset (the local one is a level-1 Warrior); the tables are
// read-only afterwards.
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "game_mode.hpp"
#include "headless_mode.hpp"
#include "items.h"
#include "multi.h"
#include "pack.h"
#include "player.h"
#include "tables/itemdat.h"
#include "tables/monstdat.h"
#include "tables/playerdat.hpp"
#include "tables/spelldat.h"

using namespace devilution;

namespace {
template <typename T>
T FromBytes(const uint8_t *data, size_t size)
{
	T value;
	std::memset(&value, 0, sizeof(value));
	std::memcpy(&value, data, size < sizeof(value) ? size : sizeof(value));
	return value;
}
} // namespace

extern "C" int LLVMFuzzerInitialize(int *, char ***)
{
	HeadlessMode = true;
	LoadSpellData();
	LoadPlayerDataFiles();
	LoadMonsterData();
	LoadItemData();
	return 0;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
		return -1;
	const uint8_t mode = data[0];
	data += 1;
	size -= 1;

	gbIsHellfire = (mode & 0x10) != 0;
	gbIsSpawn = false;
	gbIsMultiplayer = (mode & 0x20) != 0;
	Players.clear();
	Players.resize(2);
	// A real game only has created characters in its slots (level >= 1). CreatePlayer itself needs
	// the proprietary cursor graphics, so only class and level are set.
	Players[0]._pClass = HeroClass::Warrior;
	Players[0].setCharacterLevel(1);
	MyPlayer = &Players[0];
	MyPlayerId = 0;
	Player &player = Players[1];

	switch (mode & 0x3) {
	case 0: {
		const auto pack = FromBytes<ItemPack>(data, size);
		Item item;
		UnPackItem(pack, *MyPlayer, item, gbIsHellfire);
		break;
	}
	case 1: {
		const auto pack = FromBytes<ItemNetPack>(data, size);
		Item item;
		(void)UnPackNetItem(*MyPlayer, pack, item);
		break;
	}
	case 2: {
		const auto pack = FromBytes<PlayerPack>(data, size);
		UnPackPlayer(pack, player);
		break;
	}
	default: {
		const auto pack = FromBytes<PlayerNetPack>(data, size);
		(void)UnPackNetPlayer(pack, player);
		break;
	}
	}
	return 0;
}
