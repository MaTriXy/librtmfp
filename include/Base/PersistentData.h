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

#include "Base/Thread.h"
#include "Base/Exceptions.h"
#include "Base/Packet.h"
#include <functional>
#include <deque>

namespace Base {


class PersistentData : private Thread, public virtual Object {
public:
	PersistentData(const char* name = "PersistentData") : _disableTransaction(false), Thread(name) {}

	typedef std::function<void(const std::string& path, const UInt8* value, UInt32 size)> ForEach;

	void load(Exception& ex, const std::string& rootDir, const ForEach& forEach, bool disableTransaction=false);

/*!
	Add an persitant data */
	bool add(Exception& ex, const char* path, const Packet& packet) { return newEntry(ex,path, packet); }
	bool add(Exception& ex, const std::string& path, const Packet& packet) { return newEntry(ex,path.c_str(), packet); }

	bool remove(Exception& ex, const char* path) { return newEntry(ex, path); }
	bool remove(Exception& ex, const std::string& path) { return newEntry(ex, path.c_str()); }

	void flush() { stop(); }
	bool writing() { return running(); }

private:
	struct Entry : Packet, virtual public Object {
		Entry(const char* path, const Packet& packet) : path(path), Packet(std::move(packet)) {} // add

		Entry(const char* path) : path(path) {} // remove
		
		std::string path;
	};

	template <typename ...Args>
	bool newEntry(Exception& ex, const char* path, Args&&... args) {
		if (_disableTransaction)
			return true;
		std::lock_guard<std::mutex> lock(_mutex);
		if (!start(ex,Thread::PRIORITY_LOWEST))
			return false;
		_entries.emplace_back(new Entry(path, args ...));
		wakeUp.set();
		return true;
	}


	bool run(Exception& ex, const volatile bool& stopping);
	void processEntry(Exception& ex, Entry& entry);
	bool loadDirectory(Exception& ex, const std::string& directory, const std::string& path, const ForEach& forEach);

	std::string							_rootPath;
	std::mutex							_mutex;
	std::deque<shared<Entry>>	_entries;
	bool								_disableTransaction;
};

} // namespace Base
