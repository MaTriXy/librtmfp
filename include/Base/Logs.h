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
#include "Base/Logger.h"
#include "Base/String.h"
#include "Base/Thread.h"

namespace Base {

struct Logs : virtual Static {
	static Logger&		DefaultLogger() { static Logger Logger; return Logger; }
	static void			SetLogger(Logger& logger) { std::lock_guard<std::mutex> lock(_Mutex); _PLogger = &logger; }

	static void			SetLevel(LOG_LEVEL level) { _Level = level; }
	static LOG_LEVEL	GetLevel() { return _Level; }

	static void			SetDumpLimit(Int32 limit) { std::lock_guard<std::mutex> lock(_Mutex); _DumpLimit = limit; }
	static void			SetDump(const char* name); // if null, no dump, otherwise dump name, and if name is empty everything is dumped
	static bool			IsDumping() { return _Dumping; }

	template <typename ...Args>
    static void	Log(LOG_LEVEL level, const char* file, long line, Args&&... args) {
		if (_Level < level)
			return;
		std::lock_guard<std::mutex> lock(_Mutex);
		static Path File;
		static String Message;
		File.set(file);
		String::Assign(Message, std::forward<Args>(args)...);

		_PLogger->log(level, File, line, Message);
		if(Message.size()>0xFF) {
			Message.resize(0xFF);
			Message.shrink_to_fit();
		}
	}

	template <typename ...Args>
	static void Dump(const char* name, const UInt8* data, UInt32 size, Args&&... args) {
		if (!_Dumping)
			return;
		std::lock_guard<std::mutex> lock(_Mutex);
		if (_Dump.empty() || String::ICompare(_Dump, name) == 0)
			Dump(String(std::forward<Args>(args)...), data, size);
	}

	template <typename ...Args>
	static void DumpRequest(const char* name, const UInt8* data, UInt32 size, Args&&... args) {
		if (_DumpRequest)
			Dump(name, data, size, std::forward<Args>(args)...);
	}

	template <typename ...Args>
	static void DumpResponse(const char* name, const UInt8* data, UInt32 size, Args&&... args) {
		if (_DumpResponse)
			Dump(name, data, size, std::forward<Args>(args)...);
	}

#if defined(_DEBUG)
	// To dump easly during debugging => no name filter = always displaid even if no dump argument
	static void Dump(const UInt8* data, UInt32 size) {
		std::lock_guard<std::mutex> lock(_Mutex);
		Dump(String::Empty(), data, size);
	}
#endif


private:
	static void		Dump(const std::string& header, const UInt8* data, UInt32 size);


	static std::mutex				_Mutex;

	static std::atomic<LOG_LEVEL>	_Level;
	static Logger*					_PLogger;

	static volatile bool	_Dumping;
	static std::string		_Dump; // empty() means all dump, otherwise is a dump filter

	static volatile bool	_DumpRequest;
	static volatile bool	_DumpResponse;
	static Int32			_DumpLimit; // -1 means no limit
};

#undef ERROR
#undef DEBUG
#undef TRACE

#define DUMP(NAME,...) { if(Base::Logs::IsDumping()) Base::Logs::Dump(NAME,__VA_ARGS__); }

#define DUMP_REQUEST(NAME, DATA, SIZE, ADDRESS) { if(Base::Logs::IsDumping()) Base::Logs::DumpRequest(NAME, DATA, SIZE, NAME, " <= ", ADDRESS); }
#define DUMP_REQUEST_DEBUG(NAME, DATA, SIZE, ADDRESS) if(Logs::GetLevel() >= Base::LOG_DEBUG) DUMP_REQUEST(NAME, DATA, SIZE, ADDRESS)

#define DUMP_RESPONSE(NAME, DATA, SIZE, ADDRESS) { if(Base::Logs::IsDumping()) Base::Logs::DumpResponse(NAME, DATA, SIZE, NAME, " => ", ADDRESS); }
#define DUMP_RESPONSE_DEBUG(NAME, DATA, SIZE, ADDRESS) if(Logs::GetLevel() >= Base::LOG_DEBUG) DUMP_RESPONSE(NAME, DATA, SIZE, ADDRESS)

#define LOG(LEVEL, ...)  { if(Base::Logs::GetLevel()>=LEVEL) { Base::Logs::Log(LEVEL, __FILE__,__LINE__, __VA_ARGS__); } }

#define FATAL(...)	LOG(Base::LOG_FATAL, __VA_ARGS__)
#define CRITIC(...) LOG(Base::LOG_CRITIC, __VA_ARGS__)
#define ERROR(...)	LOG(Base::LOG_ERROR, __VA_ARGS__)
#define WARN(...)	LOG(Base::LOG_WARN, __VA_ARGS__)
#define NOTE(...)	LOG(Base::LOG_NOTE, __VA_ARGS__)
#define INFO(...)	LOG(Base::LOG_INFO, __VA_ARGS__)
#define DEBUG(...)	LOG(Base::LOG_DEBUG, __VA_ARGS__)
#define TRACE(...)	LOG(Base::LOG_TRACE, __VA_ARGS__)

#define AUTO_CRITIC(FUNCTION,...) { if((FUNCTION)) { if(ex)  WARN( __VA_ARGS__,", ", ex); } else { CRITIC( __VA_ARGS__,", ", ex) } }
#define AUTO_ERROR(FUNCTION,...) { if((FUNCTION)) { if(ex)  WARN( __VA_ARGS__,", ", ex); } else { ERROR( __VA_ARGS__,", ", ex) } }
#define AUTO_WARN(FUNCTION,...) { if((FUNCTION)) { if(ex)  WARN( __VA_ARGS__,", ", ex); } else { WARN( __VA_ARGS__,", ", ex) } }



} // namespace Base
