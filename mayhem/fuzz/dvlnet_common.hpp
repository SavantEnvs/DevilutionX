#pragma once
// Shared by the dvlnet harnesses: touch every accessor of a parsed packet, as base.cpp /
// tcp_server.cpp do when they dispatch on its type.
#include <memory>

#include "dvlnet/packet.h"

namespace fuzz {

inline void ConsumePacket(devilution::net::packet &pkt)
{
	(void)pkt.Type();
	(void)pkt.Source();
	(void)pkt.Destination();
	if (auto m = pkt.Message(); m.has_value()) (void)(*m)->size();
	(void)pkt.Turn();
	(void)pkt.Cookie();
	(void)pkt.NewPlayer();
	(void)pkt.Time();
	if (auto i = pkt.Info(); i.has_value()) (void)(*i)->size();
	(void)pkt.LeaveInfo();
}

} // namespace fuzz
