/*
Copyright 2016 Thomas Jammet
mathieu.poux[a]gmail.com
jammetthomas[a]gmail.com

This file is part of Librtmfp.

Librtmfp is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Librtmfp is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Librtmfp.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "RTMFP.h"
#include "Base/URL.h"
#include "AMF.h"
#include "Base/DNS.h"

using namespace std;
using namespace Base;

template<typename AddrType>
static AddrType& ReadAddr(BinaryReader& reader, AddrType& addr) {
	if (reader.available() < (sizeof(addr) + 2))
		memset(&addr, 0, sizeof(addr));
	else
		memcpy(&addr, reader.current(), sizeof(addr));
	reader.next(sizeof(addr));
	return addr;
}

RTMFP::AddressType RTMFP::ReadAddress(BinaryReader& reader, SocketAddress& address) {
	UInt8 type = reader.read8();
	if (type & 0x80) {
		in6_addr addr;
		ReadAddr(reader, addr);
		address.set(addr, reader.read16());
	}
	else {
		in_addr addr;
		ReadAddr(reader, addr);
		address.set(addr, reader.read16());
	}
	return address ? AddressType(type & 0x7F) : AddressType::ADDRESS_UNSPECIFIED;
}

BinaryWriter& RTMFP::WriteAddress(BinaryWriter& writer,const SocketAddress& address,AddressType type) {
	const IPAddress& host = address.host();
	if (host.family() == IPAddress::IPv6)
		writer.write8(type | 0x80);
	else
		writer.write8(type);
	NET_SOCKLEN size(host.size());
	const UInt8* bytes = (const UInt8*)host.data();
	for(NET_SOCKLEN i=0;i<size;++i)
		writer.write8(bytes[i]);
	return writer.write16(address.port());
}

UInt32 RTMFP::Unpack(BinaryReader& reader) {
	reader.reset();
	UInt32 id=0;
	for(int i=0;i<3;++i)
		id ^= reader.read32();
	reader.reset(4);
	return id;
}

void RTMFP::Pack(Buffer& buffer,UInt32 farId) {
	BinaryReader reader(buffer.data()+4, buffer.size()-4);
	BinaryWriter(buffer.data(),4).write32(reader.read32()^reader.read32()^farId);
}


Buffer& RTMFP::InitBuffer(shared<Buffer>& pBuffer, UInt8 marker) {
	pBuffer.set(6);
	return BinaryWriter(*pBuffer).write8(marker).write16(RTMFP::TimeNow()).buffer();
}

Buffer& RTMFP::InitBuffer(shared<Buffer>& pBuffer, atomic<Int64>& initiatorTime, UInt8 marker) {
	Int64 time = initiatorTime.exchange(0);
	if (!time)
		return InitBuffer(pBuffer, marker);
	time = Time::Now() - time;
	if ((time > 262140)) // because is not convertible in RTMFP timestamp on 2 bytes, 0xFFFF*RTMFP::TIMESTAMP_SCALE = 262140
		return InitBuffer(pBuffer, marker);
	pBuffer.set(6);
	return BinaryWriter(*pBuffer).write8(marker + 4).write16(RTMFP::TimeNow()).write16(RTMFP::Time(time)).buffer();
}

bool RTMFP::Send(Socket& socket, const Packet& packet, const SocketAddress& address) {
	Exception ex;
	int sent = socket.write(ex, packet, address);
	if (sent < 0) {
		DEBUG(ex);
		return false;
	}
	if (ex)
		DEBUG(ex);
	return true;
}

bool RTMFP::Engine::decode(Exception& ex, Buffer& buffer, const SocketAddress& address) {
	static UInt8 IV[KEY_SIZE];
	EVP_CipherInit_ex(_context, EVP_aes_128_cbc(), NULL, _key, IV, 0);
	int temp;
	EVP_CipherUpdate(_context, buffer.data(), &temp, buffer.data(), buffer.size());
	// Check CRC
	BinaryReader reader(buffer.data(), buffer.size());
	UInt16 crc(reader.read16());
	if (Crypto::ComputeChecksum(reader) != crc) {
		ex.set<Ex::Protocol>("Bad RTMFP CRC sum computing from ", address);
		return false;
	}
	buffer.clip(2);
	if (address)
		DUMP_REQUEST("LIBRTMFP", buffer.data(), buffer.size(), address);
	return true;
}

shared<Buffer>& RTMFP::Engine::encode(shared<Buffer>& pBuffer, UInt32 farId, const SocketAddress& address) {
	if (address)
		DUMP_RESPONSE("LIBRTMFP", pBuffer->data() + 6, pBuffer->size() - 6, address);

	int size = pBuffer->size();
	if (size > RTMFP::SIZE_PACKET)
		CRITIC("Packet exceeds 1192 RTMFP maximum size, risks to be ignored by client");
	// paddingBytesLength=(0xffffffff-plainRequestLength+5)&0x0F
	int temp = (0xFFFFFFFF - size + 5) & 0x0F;
	// Padd the plain request with paddingBytesLength of value 0xff at the end
	pBuffer->resize(size + temp);
	memset(pBuffer->data() + size, 0xFF, temp);
	size += temp;

	UInt8* data = pBuffer->data();

	// Write CRC (at the beginning of the request)
	BinaryReader reader(data, size);
	reader.next(6);
	BinaryWriter(data + 4, 2).write16(Crypto::ComputeChecksum(reader));
	// Encrypt the resulted request
	static UInt8 IV[KEY_SIZE];
	EVP_CipherInit_ex(_context, EVP_aes_128_cbc(), NULL, _key, IV, 1);
	EVP_CipherUpdate(_context, data + 4, &temp, data + 4, size - 4);

	reader.reset(4);
	BinaryWriter(data, 4).write32(reader.read32() ^ reader.read32() ^ farId);
	return pBuffer;
}

void RTMFP::ComputeAsymetricKeys(const Binary& sharedSecret, const UInt8* initiatorNonce,UInt32 initNonceSize, const UInt8* responderNonce, UInt32 respNonceSize, UInt8* requestKey,UInt8* responseKey) {

	Crypto::HMAC::SHA256(responderNonce, respNonceSize, initiatorNonce, initNonceSize, requestKey);
	Crypto::HMAC::SHA256(initiatorNonce, initNonceSize, responderNonce, respNonceSize, responseKey);
	// now doing HMAC-sha256 of both result with the shared secret DH key
	Crypto::HMAC::SHA256(sharedSecret.data(), sharedSecret.size(), requestKey, Crypto::SHA256_SIZE, requestKey);
	Crypto::HMAC::SHA256(sharedSecret.data(), sharedSecret.size(), responseKey, Crypto::SHA256_SIZE, responseKey);
}

bool RTMFP::ReadAddresses(BinaryReader& reader, PEER_LIST_ADDRESS_TYPE& addresses, SocketAddress& hostAddress, function<void(const SocketAddress&, AddressType)> onNewAddress) {

	// Read all addresses
	SocketAddress address;
	AddressType addressType;
	while (reader.available()) {

		addressType = RTMFP::ReadAddress(reader, address);
		switch (addressType & 0x0F) {
		case RTMFP::ADDRESS_LOCAL:
		case RTMFP::ADDRESS_PUBLIC: {
			if (addresses.size() >= RTMFP_MAX_ADDRESSES)
				break; // max size reached
			auto itAddress = addresses.lower_bound(address);
			if (itAddress == addresses.end() || itAddress->first != address) { // new address?
				addresses.emplace_hint(itAddress, address, addressType);
				onNewAddress(address, addressType);
			}
			break;
		}
		case RTMFP::ADDRESS_REDIRECTION:
			if (hostAddress != address) { // new address?
				hostAddress = address;
				onNewAddress(address, addressType);
			}
			break;
		case RTMFP::ADDRESS_UNSPECIFIED:
			onNewAddress(address, addressType);
			break;
		}
		
		TRACE("IP Address : ", address, " - type : ", addressType)
	}
	return !addresses.empty() || hostAddress;
}

bool RTMFP::ReadUrl(const char* url, string& host, SocketAddress& address, PEER_LIST_ADDRESS_TYPE& addresses, const shared<Buffer>& rawUrl) {

	// Get hostname, port and publication name
	size_t size(strlen(url));
	url = URL::Parse(url, size, host);

	// Generate the raw url
	BinaryWriter urlWriter(*rawUrl);
	urlWriter.write7Bit<UInt32>(strlen(url) + 1);
	urlWriter.write8('\x0A').write(url);

	// Extract the port
	size_t portPos = host.find_last_of(':'), ipv6End = host.find_last_of(']');
	if ((portPos != string::npos) && (ipv6End != string::npos) && portPos < ipv6End)
		portPos = string::npos;
	string port = (portPos != string::npos) ? host.substr(portPos + 1) : "1935";
	host = (portPos != string::npos) ? host.substr(0, portPos) : host;

	DEBUG("Trying to resolve the host address...")
	HostEntry hostEntry;
	Exception ex;
	if (!address.set(ex, host, port)) {
		if (DNS::Resolve(ex, host, hostEntry)) { // list of addresses
			for (auto& itAddress : hostEntry.addresses()) {
				if (address.set(ex, itAddress, port))
					addresses.emplace(address, RTMFP::ADDRESS_PUBLIC);
			}
			address.reset();
		}
	}
	if (!address && addresses.empty()) {
		ERROR("Unable to resolve host address from url ", url, " : ", ex)
		return false;
	}
	return true;
}

void RTMFP::WriteInvocation(AMFWriter& writer, const char* name, double callback, bool amf3) {

	writer.amf0 = true;
	writer.writeString(name, strlen(name));
	writer.writeNumber(callback);
	if (amf3) // without this condition connect or play doesn't work with AMS
		writer->write8(AMF::AMF0_NULL);
}

void RTMFP::WriteAMFState(AMFWriter& writer, const char* name, const char* code, const string& description, bool amf0, bool withoutClosing) {

	writer.amf0 = true;
	writer.beginObject();
	if (strcmp(name, "_error") == 0)
		writer.writeStringProperty("level", "error");
	else
		writer.writeStringProperty("level", "status");
	writer.writeStringProperty("code", code);
	writer.writeStringProperty("description", description);
	writer.amf0 = amf0;
	if (!withoutClosing)
		writer.endObject();
}

const char* RTMFP::Reason2String(Base::UInt8 reason) {

	switch ((CLOSE_REASON)reason) {
	case SESSION_CLOSED :
		return "Session Closed (default)";
		break;
	case INPUT_CONGESTED :
		return "Input Congested";
		break;
	case OUTPUT_CONGESTED :
		return "Output Congested";
		break;
	case KEEPALIVE_ATTEMPT :
		return "Keepalive Attempt";
		break;
	case P2P_ESTABLISHMENT :
		return "P2P Establishment Error";
		break;
	case P2P_RATE :
		return "P2P Low Rate";
		break;
	case OTHER_EXCEPTION :
		return "Other Exception";
		break;
	case P2P_PULL_TIMEOUT :
		return "Pull Congestion timeout";
		break;
	}	

	return "Unknown";
}
