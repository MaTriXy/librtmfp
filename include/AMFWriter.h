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
#include "AMF.h"
#include "Media.h"

using namespace Base;

struct AMFWriter : DataWriter, virtual Object {
	AMFWriter(Buffer& buffer, bool amf0 = false);

	bool repeat(UInt64 reference);
	void reset();

	UInt64 beginObject(const char* type = NULL);
	void   writePropertyName(const char* value);
	void   endObject() { endComplex(true); }

	UInt64 beginArray(UInt32 size);
	void   endArray() { endComplex(false); }

	UInt64 beginObjectArray(UInt32 size);

	UInt64 beginMap(Exception& ex, UInt32 size, bool weakKeys = false);
	void   endMap() { endComplex(false); }

	void   writeNumber(double value);
	void   writeString(const char* value, UInt32 size);
	void   writeBoolean(bool value);
	void   writeNull();
	UInt64 writeDate(const Date& date);
	UInt64 writeByte(const Packet& bytes);

	bool   amf0;

	Media::Data::Type  convert(Media::Data::Type type, Packet& packet);

	static AMFWriter&    Null() { static AMFWriter Null; return Null; }

private:
	void endComplex(bool isObject);

	AMFWriter() : _amf3(false), amf0(false) {} // null version

	void writeText(const char* value, UInt32 size);

	std::map<std::string, UInt32>	_stringReferences;
	std::vector<UInt8>				_references;
	UInt32							_amf0References;
	bool							_amf3;
	std::vector<bool>				_levels; // true if amf3
};
