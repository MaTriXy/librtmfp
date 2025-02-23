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
#include "FlashStream.h"

/**************************************************************
FlashConnection is linked to an as3 NetConnection
It creates FlashStream (NetStream) and handle messages on the
connection
*/
struct FlashConnection : FlashStream, virtual Base::Object {
	typedef Base::Event<bool(Base::UInt16 idStream, Base::UInt16& idMedia)> ON(StreamCreated);

	FlashConnection();
	virtual ~FlashConnection();

	// Add a new stream to the Main stream with an incremental id
	template <typename StreamType>
	void addStream(Base::shared<FlashStream>& pStream) {
		addStream<StreamType>((Base::UInt16)_streams.size(), pStream);
	}

	// Add a new stream to the Main stream
	template <typename StreamType>
	void addStream(Base::UInt16 id, Base::shared<FlashStream>& pStream);

	FlashStream* getStream(Base::UInt16 id, Base::shared<FlashStream>& pStream);

	// Send the stream creation request (before play or publish)
	void createStream(); // TODO: refactorize the stream creation
	
private:
	virtual bool	messageHandler(const std::string& name, AMFReader& message, Base::UInt64 flowId, Base::UInt64 writerId, double callbackHandler);
	virtual bool	rawHandler(Base::UInt16 type, const Base::Packet& packet);

	std::map<Base::UInt16, Base::shared<FlashStream>>	_streams;

	bool			_creatingStream; // If we are waiting for a stream to be created
};
