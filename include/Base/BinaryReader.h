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
#include "Base/Byte.h"
#include "Base/Binary.h"

namespace Base {


struct BinaryReader : Binary, virtual Object {

	BinaryReader(const UInt8* data, UInt32 size, Byte::Order byteOrder = Byte::ORDER_NETWORK);

	UInt8*			read(UInt32 size, UInt8* buffer);
	char*			read(UInt32 size, char*  buffer) { return STR read(size,BIN buffer); }
	template<typename BufferType>
	BufferType&		read(UInt32 size, BufferType& buffer) {
		buffer.resize(size);
		read(size,BIN buffer.data());
		return buffer;
	}

	char			read() { return _current==_end ? 0 : *_current++; }

	UInt32			read7BitValue();
	UInt64			read7BitLongValue();
	UInt32			read7BitEncoded();
	std::string&	readString(std::string& value) { return read(read7BitEncoded(),value); }
	UInt8			read8() { return _current==_end ? 0 : *_current++; }
	UInt16			read16();
	UInt32			read24();
	UInt32			read32();
	UInt64			read64();
	double			readDouble();
	float			readFloat();
	bool			readBool() { return _current==_end ? false : ((*_current++) != 0); }

	
	UInt32			position() const { return _current-_data; }
	UInt32			next(UInt32 count = 1);
	void			reset(UInt32 position = 0) { _current = _data+(position > _size ? _size : position); }
	UInt32			shrink(UInt32 available);

	const UInt8*	current() const { return _current; }
	UInt32			available() const { return _end-_current; }

	// beware, data() can be null
	const UInt8*	data() const { return _data; }
	UInt32			size() const { return _size; }

	
	static BinaryReader Null;
private:
	
	bool			_flipBytes;
	const UInt8*	_data;
	const UInt8*	_end;
	const UInt8*	_current;
	UInt32			_size;
};


} // namespace Base
