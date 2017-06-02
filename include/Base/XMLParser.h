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
#include "Base/Parameters.h"
#include "Base/Exceptions.h"
#include <vector>


namespace Base {


class XMLParser : public virtual Object {
private:

	struct Tag {
		Tag() : name(NULL), size(0), full(false) {}
		char*		name;
		UInt32		size;
		bool		full;
	};

	//// TO OVERRIDE /////

	virtual bool onStartXMLDocument() { return true; }
	
	virtual bool onXMLInfos(const char* name, Parameters& attributes) { return true; }

	virtual bool onStartXMLElement(const char* name, Parameters& attributes) = 0;
	virtual bool onInnerXMLElement(const char* name, const char* data, UInt32 size) = 0;
	virtual bool onEndXMLElement(const char* name) = 0;

	virtual void onEndXMLDocument(const char* error) {}

	/////////////////////

public:

	enum RESULT {
		RESULT_DONE,
		RESULT_PAUSED,
		RESULT_ERROR
	};

	struct XMLState : virtual NullableObject {
		friend class XMLParser;

		XMLState() : _current(NULL) {}
		operator bool() const { return _current ? true : false; }
		void clear() { _current = NULL; }
	private:
		bool						_started;
		Exception					_ex;
		const char*					_current;
		std::vector<Tag>			_tags;
	};

	void reset();
	void reset(const XMLState& state);
	void save(XMLState& state);

	RESULT parse(Exception& ex);

protected:

	XMLParser(const char* data, UInt32 size) : _started(false),_reseted(false),_current(data),_running(false),_start(data),_end(data+size) {}

private:

	RESULT		parse();
	const char*	parseXMLName(const char* endMarkers, UInt32& size);

	
	Parameters					_attributes;
	bool						_running;
	bool						_reseted;
	Buffer						_bufferInner;

	// state
	bool						_started;
	Exception					_ex;
	const char*					_current;
	std::vector<Tag>			_tags;

	// const
	const char*					_start;
	const char*					_end;
};


} // namespace Base
