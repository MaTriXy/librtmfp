/*
This code is in part based on code from the POCO C++ Libraries,
licensed under the Boost software license :
https://www.boost.org/LICENSE_1_0.txt

This file is a part of MonaSolutions Copyright 2017
mathieu.poux[a]gmail.com
jammetthomas[a]gmail.com

This program is free software: you can redistribute it and/or
modify it under the terms of the the Mozilla Public License v2.0.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
Mozilla Public License v. 2.0 received along this program for more
details (or else see http://mozilla.org/MPL/2.0/).

*/

#pragma once

#include "Base/Mona.h"
#include "Base/Net.h"
#include "Base/BinaryReader.h"

//
// Automatically link Base library.
//
#if defined(_MSC_VER)
#pragma comment(lib, "iphlpapi.lib")
#endif


namespace Base {

/*!
This class represents an internet (IP) host
address. The address can belong either to the
IPv4 or the IPv6 address family.

Relational operators (==, !=, <, <=, >, >=) are
supported. However, you must not interpret any
special meaning into the result of these 
operations, other than that the results are
consistent.

Especially, an IPv4 address is never equal to
an IPv6 address, even if the IPv6 address is
IPv4 compatible and the addresses are the same.

IPv6 addresses are supported only if the target platform
supports IPv6. */
struct IPAddress : virtual Object {
	CONST_STRING(toString());
	NULLABLE(isWildcard())

	enum Family {
		IPv4=AF_INET,
		IPv6=AF_INET6
	};

	// Creates a wildcard (zero) IPv4 IPAddress.
	IPAddress(Family family=IPv4);

	IPAddress(const IPAddress& other);
	IPAddress& set(const IPAddress& other) { return set(other, port()); }
	IPAddress& operator=(const IPAddress& other) { return set(other); }
	IPAddress& operator=(std::nullptr_t) { return reset(); }

	// Create/Set an IPAddress from a native internet address. A pointer to a in_addr or a in6_addr structure may be  passed. Additionally, for an IPv6 address, a scope ID may be specified.
	IPAddress(const in_addr& addr);
	IPAddress(const in6_addr& addr, UInt32 scope = 0);
	IPAddress& set(const in_addr& addr) { return set(addr, port()); }
	IPAddress& set(const in6_addr& addr, UInt32 scope = 0) { return set(addr, scope, port()); }

	// Set an IPAddress from binary data
	IPAddress(BinaryReader& reader, Family family = IPv4);
	IPAddress& set(BinaryReader& reader, Family family = IPv4);

	// Set an IPAddress from the string containing an IP address in presentation format (dotted decimal for IPv4, hex string for IPv6).
	bool set(Exception& ex, const char* address) { return set(ex, address, port()); }
	bool set(Exception& ex, const std::string& address) { return set(ex, address.c_str()); }
	bool setWithDNS(Exception& ex, const char* address)  { return setInternWithDNS(ex,address); }
	bool setWithDNS(Exception& ex, const std::string& address)  { return setInternWithDNS(ex,address.c_str()); }

	// Set an IPAddress from the string containing an IP address in presentation format (dotted decimal for IPv4, hex string for IPv6).
	bool set(Exception& ex, const char* address, Family family) { return set(ex, address, family, port()); }
	bool set(Exception& ex, const std::string& address, Family family) { return set(ex, address.c_str(), family); }

	IPAddress& reset();

	// Masks the IP address using the given netmask, which is usually a IPv4 subnet mask (Only supported for IPv4 addresses)
	// The new address is (address & mask)
	bool mask(Exception& ex, const IPAddress& mask) { return this->mask(ex, mask, Wildcard());}
	// Masks the IP address using the given netmask, which is usually a IPv4 subnet mask (Only supported for IPv4 addresses)
	// The new address is (address & mask) | (set & ~mask)
	bool mask(Exception& ex, const IPAddress& mask, const IPAddress& set);


	// Returns the address family (IPv4 or IPv6) of the address
	Family family() const;
	
	// Returns the IPv6 scope identifier of the address. Returns 0 if the address is an IPv4 address
	UInt32 scope() const;

	// Returns true iff the address is a wildcard (all zero) address
	bool isWildcard() const;
		
	// Returns true if the address is a broadcast address (Only IPv4 addresses can be broadcast addresse)
	bool isBroadcast() const;
	bool isAnyBroadcast() const;
	
	// Returns true if the address is a loopback address
	// 127.0.0.1 for IPv4, ::1 for IPv6
	bool isLoopback() const;
	
	// Returns true if the address is a multicast address
	// 224.0.0.0 to 239.255.255.255 for IPv4, FFxx:x:x:x:x:x:x:x range for IPv6
	bool isMulticast() const;
		
	// Returns true if the address is a unicast address (if it is neither a wildcard, broadcast or multicast address)
	bool isUnicast() const { return !isWildcard() && !isAnyBroadcast() && !isMulticast(); }
		
	// Returns true if the address is a link local unicast address
	// 169.254.0.0/16 range for IPv4, 1111 1110 10 as the first 10 bits followed by 54 zeros for IPv6
	bool isLinkLocal() const;
		
	// Returns true if the address is a site local unicast address
	// 10.0.0.0/24, 192.168.0.0/16 or 172.16.0.0 to 172.31.255.255 ranges for IPv4, 1111 1110 11 as the first 10 bits, followed by 38 zeros for IPv6
	bool isSiteLocal() const;
		
	// Returns true if the address is IPv4 compatible (for IPv6 the address must be in the ::x:x range)
	bool isIPv4Compatible() const;

	// Returns true if the address is an IPv4 mapped IPv6 address (For IPv6, the address must be in the ::FFFF:x:x range)
	bool isIPv4Mapped() const;

	// Returns true if the address is a well-known multicast address
	// 224.0.0.0/8 range for IPv4, FF0x:x:x:x:x:x:x:x range for IPv6
	bool isWellKnownMC() const;
	
	// Returns true if the address is a node-local multicast address
	// Always false for IPv4, node-local multicast addresses are in the FFx1:x:x:x:x:x:x:x range for IPv6
	bool isNodeLocalMC() const;

	// Returns true if the address is a link-local multicast address
	// 224.0.0.0/24 range for IPv4, FFx2:x:x:x:x:x:x:x range for IPv6
	bool isLinkLocalMC() const;

	// Returns true if the address is a site-local multicast address
	// 239.255.0.0/16 range for IPv4, FFx5:x:x:x:x:x:x:x for IPv6
	bool isSiteLocalMC() const;

	// Returns true if the address is a organization-local multicast address
	// 239.192.0.0/16 range for IPv4, FFx8:x:x:x:x:x:x:x range for IPv6
	bool isOrgLocalMC() const;

	// Returns true if the address is a global multicast address
	// 224.0.1.0 to 238.255.255.255 range for IPv4, FFxF:x:x:x:x:x:x:x range for IPv6
	bool isGlobalMC() const;

	// Returns the prefix length
	UInt8 prefixLength() const;

	// Returns true if the address is local
	bool isLocal() const { return isWildcard() || isLoopback() || isSiteLocal() || isLinkLocal() || isAnyBroadcast() || isNodeLocalMC() || isLinkLocalMC() || isSiteLocalMC() || isOrgLocalMC(); }
	
	bool operator == (const IPAddress& addr) const;	
	bool operator != (const IPAddress& addr) const { return !operator==(addr); }
	bool operator <  (const IPAddress& addr) const;
	bool operator <= (const IPAddress& addr) const { return operator==(addr) || operator<(addr); }
	bool operator >  (const IPAddress& addr) const { return !operator<=(addr); }
	bool operator >= (const IPAddress& addr) const { return operator==(addr) || operator>(addr); }
	
	// Returns the internal address structure
	const void* data() const;
	UInt8		size() const;

	// Returns a wildcard IPv4 or IPv6 address (0.0.0.0)
	static const IPAddress& Wildcard(Family family = IPv4);
	// Returns a loopback address, 127.0.0.1 for IPv4, ::1 for IPv6
	static const IPAddress& Loopback(Family family = IPv4);
	// Returns a broadcast IPv4 address (255.255.255.255)
	static const IPAddress& Broadcast();

	static bool Resolve(Exception& ex, const char* address, IPAddress& host);

	/*!
	Returns a list for all local IP addresses
	/!\ expensive cost and time method */
	static bool GetLocals(Exception& ex, std::vector<IPAddress>& locals);
protected:
	IPAddress(const sockaddr& addr);
	IPAddress(const IPAddress& other, UInt16 port);

	IPAddress& set(const in_addr& addr, UInt16 port);
	IPAddress& set(const in6_addr& addr, UInt32 scope, UInt16 port);

	IPAddress& set(const sockaddr& addr);
	IPAddress& set(const IPAddress& other, UInt16 port);

	bool set(Exception& ex, const char* address, UInt16 port);
	bool set(Exception& ex, const char* address, Family family, UInt16 port);
	bool setWithDNS(Exception& ex, const char* address, UInt16 port) { return setInternWithDNS(ex,address,port); }

	void setPort(UInt16 port);

	UInt16				port() const;
	const sockaddr_in6&	addr() const;

	const std::string&	addrToString() const;

private:
	const std::string& toString() const;

	struct IPImpl;
	struct IPv4Impl;
	struct IPv6Impl;

	IPAddress(shared<IPImpl>&& pIPAddress);

	template <typename ...Args>
	bool setInternWithDNS(Exception& ex, const char* address, Args&&... args) {
		if (set(ex, address, args ...))
			return true;
		ex = nullptr;
		IPAddress host;
		if (!Resolve(ex, address, host))
			return false;
		set(host, args ...);
		return true;
	}

	shared<IPImpl> _pIPAddress;
};



} // namespace Base
