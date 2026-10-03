/****
 * Sming Framework Project - Open Source framework for high efficiency native ESP8266 development.
 * Created 2015 by Skurydin Alexey
 * http://github.com/SmingHub/Sming
 * All files of the Sming Core are provided under the LGPL v3 license.
 *
 * Server.cpp
 *
 ****/

#include "include/Network/Mdns/Server.h"
#include "Packet.h"
#include <Platform/Station.h>
#include <debug_progmem.h>

namespace mDNS
{
Server server;

Server::~Server()
{
	if(active) {
		UdpConnection::leaveMulticastGroup(MDNS_IP);
	}
}

bool Server::search(const String& name, ResourceType type)
{
	Request req(Request::Type::query);
	req.addQuestion(name, type);
	return send(req);
}

bool Server::send(Message& message)
{
	if(sendCallback) {
		sendCallback(message);
	}

	auto buf = reinterpret_cast<const char*>(message.getData());
	auto len = message.getSize();

	begin();
	return sendTo(message.getRemoteIp(), message.getRemotePort(), buf, len);
}

bool Server::begin()
{
	if(active) {
		return true;
	}

	auto localIp = WifiStation.getIP();

	if(!joinMulticastGroup(localIp, MDNS_IP)) {
		debug_w("[mDNS] joinMulticastGroup() failed");
		return false;
	}

	if(!listen(MDNS_SOURCE_PORT)) {
		debug_e("[mDNS] listen failed");
		return false;
	}

	setMulticast(localIp);
	setMulticastTtl(MDNS_TTL);

	active = true;
	return true;
}

void Server::end()
{
	if(!active) {
		return;
	}

	close();
	leaveMulticastGroup(MDNS_IP);
	active = false;
}

void Server::onReceive(pbuf* buf, IpAddress remoteIP, uint16_t remotePort)
{
	// A large reply can arrive as a chain of pbufs, but the Message parser treats
	// the payload as one flat buffer and walks name/record offsets across the
	// whole datagram. Using only the first pbuf (buf->payload/buf->len) would
	// read those offsets past the segment into unrelated adjacent heap, silently
	// corrupting decoded names. Flatten the chain into one contiguous buffer.
	uint8_t* data = static_cast<uint8_t*>(buf->payload);
	uint16_t len = buf->len;
	uint8_t* linearBuf = nullptr;
	if(buf->tot_len > buf->len) {
		// RFC 6762 §17 caps an mDNS message at 9000 bytes; reject anything larger so a
		// malformed/oversized datagram cannot force a large transient heap allocation.
		if(buf->tot_len > MDNS_MAX_MESSAGE_SIZE) {
			debug_w("[mDNS] onReceive: dropping oversized %u-byte packet", buf->tot_len);
			return;
		}
		linearBuf = new uint8_t[buf->tot_len];
		if(linearBuf == nullptr) {
			debug_e("[mDNS] onReceive: out of memory flattening %u-byte packet", buf->tot_len);
			return;
		}
		pbuf_copy_partial(buf, linearBuf, buf->tot_len, 0);
		data = linearBuf;
		len = buf->tot_len;
	}

	if(packetCallback) {
		if(!packetCallback(remoteIP, remotePort, data, len)) {
			delete[] linearBuf;
			return;
		}
	}

	if(handlers.isEmpty()) {
		delete[] linearBuf;
		return;
	}

	Message message(remoteIP, remotePort, data, len);
	if(message.parse()) {
		for(auto& handler : handlers) {
			if(!handler.onMessage(message)) {
				break;
			}
		}
	}

	delete[] linearBuf;
}

} // namespace mDNS
