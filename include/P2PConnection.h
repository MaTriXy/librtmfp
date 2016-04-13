
#pragma once

#include "FlowManager.h"
#include "Mona/StopWatch.h"

#define COOKIE_SIZE	0x40

/**************************************************
P2PConnection represents a direct P2P connection 
with another peer
*/
class P2PConnection : public FlowManager {
	friend class RTMFPConnection;
public:
	P2PConnection(FlowManager& parent, std::string id, Invoker* invoker, OnSocketError pOnSocketError, OnStatusEvent pOnStatusEvent, OnMediaEvent pOnMediaEvent, const Mona::SocketAddress& hostAddress, const Mona::Buffer& pubKey, bool responder);

	virtual ~P2PConnection();

	virtual Mona::UDPSocket&	socket() { return _parent.socket(); }

	// Add a command to the main stream (play/publish/netgroup)
	virtual void addCommand(CommandType command, const char* streamName, bool audioReliable = false, bool videoReliable = false);

	// Return listener if started successfully, otherwise NULL (only for RTMFP connection)
	virtual Listener* startListening(Mona::Exception& ex, const std::string& streamName, const std::string& peerId, FlashWriter& writer);

	// Remove the listener with peerId (only for RTMFP connection)
	virtual void stopListening(const std::string& peerId);

	// Set the p2p publisher as ready (used for blocking mode)
	virtual void setP2pPublisherReady();

	Mona::UInt8						attempt; // Number of try to contact the responder (only for initiator)
	Mona::Stopwatch					lastTry; // Last time handshake 30 has been sent to the server (only for initiator)

	std::string						peerId; // Peer Id of the peer connected
	static Mona::UInt32				P2PSessionCounter; // Global counter for generating incremental P2P sessions id

	// Set the tag used for this connection (responder mode)
	void setTag(const std::string& tag) { _tag = tag; }

	// Set the group id
	void setGroupId(const std::string& groupHex, const std::string& groupTxt, const std::string& streamName) { _groupHex = groupHex; _groupTxt = groupTxt; _streamName = streamName; }

	// Return the tag used for this p2p connection (initiator mode)
	std::string	getTag() { return _tag; }

	// Manage all handshake messages (marker 0x0B)
	virtual void manageHandshake(Mona::Exception& ex, Mona::BinaryReader& reader);

	// Handle the first P2P responder handshake message (called by RTMFPConnection)
	void responderHandshake0(Mona::Exception& ex, std::string tag, const Mona::SocketAddress& address);

	// Handle the second P2P responder handshake message
	void responderHandshake1(Mona::Exception& ex, Mona::BinaryReader& reader);

	// Send the second P2P initiator handshake message in a middle mode (local)
	void initiatorHandshake70(Mona::Exception& ex, Mona::BinaryReader& reader, const Mona::SocketAddress& address);

	// Send the third P2P initiator handshake message
	bool initiatorHandshake2(Mona::Exception& ex, Mona::BinaryReader& reader);

	// Flush the connection
	// marker values can be :
	// - 0B for handshake
	// - 0A for raw response in P2P mode (only for responder)
	// - 8A for AMF responde in P2P mode (only for responder)
	// - 4A for acknowlegment in P2P mode (TODO: see if it is needed)
	virtual void				flush(bool echoTime, Mona::UInt8 marker);

	// Does the connection is terminated? => can be deleted by parent
	bool consumed() { return _died; }

protected:
	// Handle stream creation (only for RTMFP connection)
	virtual void				handleStreamCreated(Mona::UInt16 idStream);

	// Handle play request (only for P2PConnection)
	virtual bool				handlePlay(const std::string& streamName, FlashWriter& writer);

	// Handle new peer in a Netgroup : connect to the peer (only for RTMFPConnection)
	virtual void				handleNewGroupPeer(const std::string& groupId, const std::string& peerId);

	// Handle a NetGroup connection message from a peer connected (only for P2PConnection)
	virtual void				handleGroupHandshake(const std::string& groupId, const std::string& key, const std::string& id);

	// Handle a P2P address exchange message (Only for RTMFPConnection)
	virtual void				handleP2PAddressExchange(Mona::Exception& ex, Mona::PacketReader& reader);
	
	// Close the conection properly
	virtual void				close();

private:
	FlowManager&				_parent; // RTMFPConnection related to
	Mona::UInt32				_sessionId; // id of the P2P session;
	std::string					_farKey; // Key of the server/peer
	std::string					_farNonce; // Nonce of the distant peer

	// Play/Publish command
	std::string					_streamName; // playing stream name
	std::string					_groupHex; // Group ID encrypted (double sha256) in Hex format
	std::string					_groupTxt; // Group ID in plain text (without ending zeroes)
	bool						_responder; // is responder?

	bool						_rawResponse; // next message is a raw response? TODO: make it nicer
	bool						_groupConnectSent; // True if group connection request has been sent to peer

	FlashStream::OnGroupMedia::Type	onGroupMedia; // Called when a peer signal us that their is a stream available in the NetGroup
};