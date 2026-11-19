// One multiplayer datagram (Source/dvlnet/packet.*): packet_factory::make_packet is what every
// transport (TCP, ZeroTier, loopback) calls on received bytes. Byte 0 bit 0 selects a
// password-protected game (libsodium secretbox Decrypt path) vs a public one (plain Create).
#include <cstddef>
#include <cstdint>
#include <string>

#include "dvlnet/packet.h"
#include "dvlnet_common.hpp"

using namespace devilution::net;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	if (size < 1)
		return -1;
	const bool secure = (data[0] & 1) != 0;
	buffer_t buf(data + 1, data + size);
	// Factories are rebuilt per input: no state survives between inputs.
	packet_factory factory = secure ? packet_factory(std::string("fuzzpass")) : packet_factory();
	auto pkt = factory.make_packet(std::move(buf));
	if (pkt.has_value())
		fuzz::ConsumePacket(**pkt);
	return 0;
}
