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


#include "Base/TLS.h"


using namespace std;

namespace Base {

bool TLS::Create(Exception& ex, shared<TLS>& pTLS, const SSL_METHOD* method) {
	// load and configure in constructor to be thread safe!
	SSL_CTX* pCTX(SSL_CTX_new(method));
	if (pCTX) {
		/* If the underlying BIO is blocking, SSL_read()/SSL_write() will only return, once the read operation has been finished or an error occurred,
		except when a renegotiation take place, in which case a SSL_ERROR_WANT_READ may occur.
		This behaviour can be controlled with the SSL_MODE_AUTO_RETRY flag of the SSL_CTX_set_mode call. */
		SSL_CTX_set_mode(pCTX, SSL_MODE_AUTO_RETRY);
		pTLS.reset(new TLS(pCTX));
		return true;
	}
	ex.set<Ex::Extern::Crypto>(Crypto::LastErrorMessage());
	return false;
}

bool TLS::Create(Exception& ex, const char* cert, const char* key, shared<TLS>& pTLS, const SSL_METHOD* method) {
	// load and configure in constructor to be thread safe!
	SSL_CTX* pCTX(SSL_CTX_new(method));
	if (pCTX) {
		// cert.pem & key.pem
		if (SSL_CTX_use_certificate_file(pCTX, cert, SSL_FILETYPE_PEM) == 1 && SSL_CTX_use_PrivateKey_file(pCTX, key, SSL_FILETYPE_PEM) == 1) {
			/* If the underlying BIO is blocking, SSL_read()/SSL_write() will only return, once the read operation has been finished or an error occurred,
			except when a renegotiation take place, in which case a SSL_ERROR_WANT_READ may occur.
			This behaviour can be controlled with the SSL_MODE_AUTO_RETRY flag of the SSL_CTX_set_mode call. */
			SSL_CTX_set_mode(pCTX, SSL_MODE_AUTO_RETRY);
			pTLS.reset(new TLS(pCTX));
			return true;
		}
		SSL_CTX_free(pCTX);
	}
	ex.set<Ex::Extern::Crypto>(Crypto::LastErrorMessage());
	return false;
}

TLS::Socket::Socket(Type type, const shared<TLS>& pTLS) : pTLS(pTLS), Base::Socket(type), _ssl(NULL) {}

TLS::Socket::Socket(NET_SOCKET sockfd, const sockaddr& addr, const shared<TLS>& pTLS) : pTLS(pTLS), Base::Socket(sockfd, addr), _ssl(NULL) {}

TLS::Socket::~Socket() {
	if (_ssl)
		SSL_free(_ssl);
}

UInt32 TLS::Socket::available() const {
	UInt32 available = Base::Socket::available();
	if (!pTLS)
		return available; // normal socket
	lock_guard<mutex> lock(_mutex);
	if (!_ssl)
		return available;
	// don't call SSL_pending while handshake, it create a "wrong error 'don't call this function'"
	if (inHandshake())
		return available ? 0x4000 : 0; // max buffer possible size
	if (available)
		return 0x4000 + SSL_pending(_ssl);  // max buffer possible size and has to read!
	return SSL_pending(_ssl); // max buffer possible size (nothing to read)
}

Base::Socket* TLS::Socket::newSocket(Exception& ex, NET_SOCKET sockfd, const sockaddr& addr) {
	if(!pTLS)
		return Base::Socket::newSocket(ex, sockfd, addr); // normal socket

	Socket* pSocket(new Socket(sockfd, addr, pTLS));

	pSocket->_ssl = SSL_new(pTLS->_pCTX);
	if (!pSocket->_ssl || SSL_set_fd(pSocket->_ssl, *pSocket) != 1) {
		// Certainly a TLS error context
		ex.set<Ex::Extern::Crypto>(Crypto::LastErrorMessage());
		delete pSocket;
		return NULL;
	}
	SSL_set_accept_state(pSocket->_ssl);
	if (pSocket->catchResult(ex, SSL_do_handshake(pSocket->_ssl), " (peer=", pSocket->peerAddress(), ")") >= 0)
		return pSocket;
	if (ex.cast<Ex::Net::Socket>().code != NET_EWOULDBLOCK) {
		delete pSocket;
		return NULL;
	}
	ex = nullptr;
	return pSocket;
}

bool TLS::Socket::connect(Exception& ex, const SocketAddress& address, UInt16 timeout) {
	bool result = Base::Socket::connect(ex, address, timeout);
	if (!pTLS)
		return result;  // normal socket or already doing SSL negociation!

	bool connected(true);
	if (!result) {
		if (ex.cast<Ex::Net::Socket>().code != NET_EWOULDBLOCK)
			return false;
		ex = nullptr;
		connected = false;
	}

	lock_guard<mutex> lock(_mutex);
	if (_ssl) // already connected!
		return result;

	_ssl = SSL_new(pTLS->_pCTX);
	if (!_ssl || SSL_set_fd(_ssl, *this) != 1) {
		// Certainly a TLS error context
		ex.set<Ex::Extern::Crypto>(Crypto::LastErrorMessage());
		return false;
	}
	
	SSL_set_connect_state(_ssl);
	return connected ? catchResult(ex, SSL_do_handshake(_ssl), " (address=", address, ")") >= 0 : true;
}

int TLS::Socket::receive(Exception& ex, void* buffer, UInt32 size, int flags, SocketAddress* pAddress) {
	if (!pTLS)
		return Base::Socket::receive(ex, buffer, size, flags, pAddress); // normal socket

	lock_guard<mutex> lock(_mutex);
	if (!_ssl)
		return Base::Socket::receive(ex, buffer, size, flags, pAddress); // normal socket
	int result = catchResult(ex, SSL_read(_ssl, buffer, size), " (from=", peerAddress(), ", size=", size, ", flags=", flags, ")");

	// assign pAddress (no other way possible here)
	if(pAddress)
		pAddress->set(peerAddress());
	if (result > 0)
		Base::Socket::receive(result);
	return result;
}

int TLS::Socket::sendTo(Exception& ex, const void* data, UInt32 size, const SocketAddress& address, int flags) {
	if (!pTLS)
		return Base::Socket::sendTo(ex, data, size, address, flags); // normal socket
	lock_guard<mutex> lock(_mutex);
	if (!_ssl)
		return Base::Socket::sendTo(ex, data, size, address, flags); // normal socket
	int result = catchResult(ex, SSL_write(_ssl, data, size), " (address=", address ? address : peerAddress(), ", size=", size, ", flags=", flags, ")");
	if (result > 0)
		Base::Socket::send(result);
	return result;
}

UInt64 TLS::Socket::queueing() const {
	if (!pTLS)
		return Base::Socket::queueing();
	lock_guard<mutex> lock(_mutex);
	if (_ssl && inHandshake())
		return Base::Socket::queueing() + 1;
	return Base::Socket::queueing();
}

bool TLS::Socket::flush(Exception& ex) {
	// Call when Writable!
	if (!pTLS || Base::Socket::queueing()) // if queueing a SLL_Write will do the handshake!
		return Base::Socket::flush(ex);
	// maybe WRITE event for handshake need!
	lock_guard<mutex> lock(_mutex);
	if (!_ssl)
		return Base::Socket::flush(ex);
	int result(catchResult(ex, SSL_do_handshake(_ssl)));
	if (result >= 0) // handshake success
		return Base::Socket::flush(ex); // try to flush after hanshake
	if (ex.cast<Ex::Net::Socket>().code != NET_EWOULDBLOCK)
		return false;
	ex = nullptr;
	return true;
}

bool TLS::Socket::inHandshake() const{
	_ssl->method->ssl_renegotiate_check(_ssl);
	return (SSL_state(_ssl)&SSL_ST_INIT) ? true : false;
}


struct SSLInitializer {
	SSLInitializer() {
		SSL_library_init(); // can't fail!
		ERR_load_BIO_strings();
		SSL_load_error_strings();
		OpenSSL_add_all_algorithms();
	}
	~SSLInitializer() { EVP_cleanup(); }
};
SSLInitializer _Initializer;

}  // namespace Base
