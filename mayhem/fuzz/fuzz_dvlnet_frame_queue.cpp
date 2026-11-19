// TCP multiplayer stream (Source/dvlnet/frame_queue.cpp + packet.cpp): the receive loop of
// tcp_server.cpp/tcp_client.cpp. The input is split into socket reads (each chunk is prefixed by
// one length byte), written into a frame_queue, and every complete frame is handed to
// packet_factory::make_packet, exactly as handle_recv does.
#include <cstddef>
#include <cstdint>

#include "dvlnet/frame_queue.h"
#include "dvlnet/packet.h"
#include "dvlnet_common.hpp"

using namespace devilution::net;

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
	frame_queue queue;
	packet_factory factory;
	size_t pos = 0;
	while (pos < size) {
		size_t chunk = static_cast<size_t>(data[pos++]) + 1;
		if (chunk > size - pos)
			chunk = size - pos;
		queue.Write(buffer_t(data + pos, data + pos + chunk));
		pos += chunk;
		while (true) {
			auto ready = queue.PacketReady();
			if (!ready.has_value())
				return 0; // tcp_server drops the connection on a bad frame
			if (!*ready)
				break;
			auto frame = queue.ReadPacket();
			if (!frame.has_value())
				return 0;
			auto pkt = factory.make_packet(*frame);
			if (pkt.has_value())
				fuzz::ConsumePacket(**pkt);
		}
	}
	return 0;
}
