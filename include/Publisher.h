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

#include "Base/Mona.h"
#include "FlashWriter.h"
#include "DataReader.h"
#include "Base/Packet.h"
#include <deque>

struct Invoker;
class Listener;
struct Publisher : virtual Base::Object {

	Publisher(const std::string& name, Invoker& invoker, bool audioReliable, bool videoReliable, bool p2p);
	virtual ~Publisher();
	const std::string&		name() const { return _name; }

	void					start();
	bool					running() const { return _running; }
	void					stop();
	Base::UInt32			count() const { return _listeners.size(); }

	template <typename ListenerType, typename... Args>
	ListenerType*				addListener(Base::Exception& ex, const std::string& identifier, Args... args) {
		auto it = _listeners.lower_bound(identifier);
		if (it != _listeners.end() && it->first == identifier) {
			ex.set<Base::Ex::Application>("Already subscribed to ", _name);
			return NULL;
		}
		if (it != _listeners.begin())
			--it;
		ListenerType* pListener = new ListenerType(*this, identifier, args...);
		_listeners.emplace_hint(it, identifier, pListener);
		return pListener;
	}
	void					removeListener(const std::string& identifier);

	const Base::Packet&		audioCodecBuffer() const { return _audioCodec; }
	const Base::Packet&		videoCodecBuffer() const { return _videoCodec; }

	// Functions called by RTMFPSession
	void pushAudio(Base::UInt32 time, const Base::Packet& packet);
	void pushVideo(Base::UInt32 time, const Base::Packet& packet);
	void pushData(Base::UInt32 time, const Base::Packet& packet);
	void flush();

	bool	isP2P; // If true it is a p2p publisher
private:
	// Update and check the time synchronization variables
	void updateTime(AMF::Type type, Base::UInt32 time, Base::UInt32 size);

	bool publishAudio;
	bool publishVideo;

	Invoker&							_invoker;
	bool								_running; // If the publication is running
	std::map<std::string, Listener*>	_listeners; // list of listeners to this publication
	const std::string					_name; // name of the publication

	bool								_videoReliable;
	bool								_audioReliable;

	Base::Packet						_audioCodec;
	Base::Packet						_videoCodec;
	bool								_new; // True if there is at list a packet to send

	// Synchronisation checks
	Base::UInt32						_lastTime; // last time received
	Base::Time							_lastSyncWarn; // Time since last synchronisation issue
	Base::Time							_lastPacket; // last time we received a packet

	// This class is used to detect time jump and congestion from publisher
	struct TimeJump {
		TimeJump() : _cumulatedTime(0), _lastTime(0), _bytes(0) {}

		// Update the time jump/congestion state
		// return: 0 if we received less than 1,5s last second, the number of ms received
		Base::Int64 operator()(Base::UInt32 time, Base::UInt32 size, Base::UInt64& bytes);
	private:
		Base::Time		_lastSecond;
		Base::UInt64	_cumulatedTime; // cumulated number of second received
		Base::UInt32	_lastTime; // last time received
		Base::UInt64	_bytes; // number of bytes received last second
	};
	TimeJump							_audioJump;
	TimeJump							_videoJump;
};
