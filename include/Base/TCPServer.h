/*
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
#include "Base/IOSocket.h"
#include "Base/TLS.h"


namespace Base {


class TCPServer : public virtual Object {
public:
	typedef Event<void(const shared<Socket>& pSocket)>  ON(Connection);
	typedef Socket::OnError								ON(Error);

	TCPServer(IOSocket& io, const shared<TLS>& pTLS=nullptr);
	virtual ~TCPServer();

	IOSocket&						io;
	const shared<Socket>&	socket() { return _pSocket; }
	Socket*							operator->() { return _pSocket.get(); }


	bool					start(Exception& ex, const SocketAddress& address);
	bool					start(Exception& ex, const IPAddress& ip=IPAddress::Wildcard()) { return start(ex, SocketAddress(ip, 0)); }
	bool					running() const { return _running;  }
	void					stop();


private:
	Socket::OnAccept		_onAccept;

	shared<Socket>	_pSocket;
	bool					_running;
};


} // namespace Base
