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

#pragma once

#include "BandWriter.h"
#include "RTMFP.h"
#include "Base/DiffieHellman.h"
#include "Base/SocketAddress.h"

struct Invoker;
struct RTMFPSession;
struct FlowManager;

// Waiting handshake request
struct Handshake : virtual Base::Object {

	Handshake(FlowManager* session, const Base::SocketAddress& host, const PEER_LIST_ADDRESS_TYPE& addresses, bool p2p, bool delayed) :
		status(RTMFP::STOPPED), pCookie(NULL), pTag(NULL), attempt(0), hostAddress(host), addresses(addresses), pSession(session), isP2P(p2p), rdvDelayed(delayed) {}

	bool					isP2P; // True if it is a p2p handshake
	const std::string*		pCookie; // pointer to the cookie buffer
	const std::string*		pTag; // pointer to the tag
	std::string				cookieReceived; // Value of the far peer cookie (initiator)
	Base::Time				cookieCreation; // Time when the cookie has been created
	FlowManager*			pSession; // Session related to (it can be null if we are in a handshake of a responder)
	RTMFP::SessionStatus	status; // State of the handshake

	PEER_LIST_ADDRESS_TYPE	addresses; // map of target addresses (peers/servers) to attempt
	Base::SocketAddress		hostAddress; // host address (server or p2p rendezvous service)

	Base::UInt8				attempt; // Counter of attempt
	Base::Time				lastTry; // Last attempt

	bool					rdvDelayed; // If true we wait 5s before starting requests to rendezvous service

	// Coding keys
	Base::shared<Base::Buffer>		farKey; // Far public key
	Base::shared<Base::Buffer>		farNonce; // Far nonce
};

/**************************************************
RTMFPHandshaker handle the socket and the map of
socket addresses to RTMFPConnection
It is the entry point for all IO
*/
struct RTMFPHandshaker : BandWriter  {

	RTMFPHandshaker(RTMFPSession* pSession);

	virtual ~RTMFPHandshaker();

	// Return the name of the session
	virtual const std::string&			name() { return _name; }

	// Start a new Handshake if possible and add it to the map of tags
	// param delayed: if True we first try to connect directly to addresses and after 5s we start to contact the rendezvous service, if False we connect to all addresses
	// return True if the connection is created
	bool								startHandshake(Base::shared<Handshake>& pHandshake, const Base::SocketAddress& address, const PEER_LIST_ADDRESS_TYPE& addresses, FlowManager* pSession, bool p2p, bool delay);
	bool								startHandshake(Base::shared<Handshake>& pHandshake, const Base::SocketAddress& address, FlowManager* pSession, bool p2p);

	// Create the handshake object if needed and send a handshake 70 to address
	void								sendHandshake70(const std::string& tag, const Base::SocketAddress& address, const Base::SocketAddress& host);

	// Called by Invoker every second to manage connection (flush and ping)
	void								manage(Base::Int64 now);

	// Close the socket all connections
	void								close();

	// Return the socket object
	virtual const Base::shared<Base::Socket>&	socket(Base::IPAddress::Family family);

	// Return true if the session has failed
	virtual bool						failed();

	// Remove the handshake properly
	void								removeHandshake(const Base::shared<Handshake> pHandshake);

	// Treat decoded message
	virtual void						receive(const Base::SocketAddress& address, const Base::Packet& packet);

private:
	enum {
		P2P_DELAY_RENDEZVOUS = 5000, // Delay before starting to send the rendezvous service requests in a NetGroup
		DELAY_MANAGE = 500	// Delay between each manage
	};

	// Send the first handshake message (with rtmfp url/peerId + tag)
	void								sendHandshake30(const Base::SocketAddress& address, const Base::Binary& epd, const std::string& tag);

	// Handle the handshake 30 (p2p concurrent connection)
	void								handleHandshake30(Base::BinaryReader& reader);

	// Handle a server redirection message or a p2p address exchange
	void								handleRedirection(Base::BinaryReader& reader);

	// Send the 2nd handshake response (only in P2P mode)
	void								sendHandshake78(Base::BinaryReader& reader);

	// Handle the handshake 70 (from peer or server)
	void								handleHandshake70(Base::BinaryReader& reader);

	// Send the 2nd handshake request
	void								sendHandshake38(const Base::shared<Handshake>& pHandshake, const std::string& cookie);

	// Send the first handshake response (only in P2P mode)
	void								sendHandshake70(const std::string& tag, Base::shared<Handshake>& pHandshake);

	// Compute the public key if not already done
	bool								computePublicKey();

	// Process current handshakes & manage cookies
	void								processManage();

	std::map<std::string, Base::shared<Handshake>>		_mapTags; // map of Tag to waiting handshake
	std::map<std::string, Base::shared<Handshake>>		_mapCookies; // map of Cookies to waiting handshake

	RTMFPSession*						_pSession; // Pointer to the main RTMFP session for assocation with new connections
	const std::string					_name; // name of the session (handshaker)
	Base::Packet						_publicKey; // Our public key (fixed for the session) TODO: see if we move it into RTMFPSession
	Base::Time							_lastManage; // timer for manage() events
};
