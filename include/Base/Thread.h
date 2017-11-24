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
#include "Base/Exceptions.h"
#include "Base/Signal.h"
#include <thread>

namespace Base {

struct Thread : virtual Object {
	enum Priority {
		PRIORITY_LOWEST=0,
		PRIORITY_LOW,
		PRIORITY_NORMAL,
		PRIORITY_HIGH,
		PRIORITY_HIGHEST
	};

	virtual ~Thread();

	bool						start(Exception& ex, Priority priority = PRIORITY_NORMAL);
	virtual void				stop();

	const char*					name() const { return _name; }
	bool						running() const { return !_stop; }
	
	static unsigned				ProcessorCount() { unsigned result(std::thread::hardware_concurrency());  return result > 0 ? result : 1; }
	
	/*!
	A sleep, usefull for test, with imprecise milliseconds resolution (resolution from 5 to 15ms) */
	static void					Sleep(UInt32 duration) { std::this_thread::sleep_for(std::chrono::milliseconds(duration)); }

	static const std::string&	CurrentName() { return _Name; }
	static UInt32				CurrentId();
	static const UInt32			MainId;

	struct ChangeName : virtual Object {
		template <typename ...Args>
		ChangeName(Args&&... args) { SetSystemName(String::Assign(_oldName, std::forward<Args>(args)...)); _oldName.swap(_Name); }
		~ChangeName() { SetSystemName(_Name = std::move(_oldName)); }
		operator const std::string&() const { return _Name; }
	private:
		std::string _oldName;
	};
protected:
	Thread(const char* name);

	Signal wakeUp;
	template <typename ...Args>
	const std::string& setName(Args&&... args) { SetSystemName(String::Assign(_Name, std::forward<Args>(args)...)); return _Name; }
private:
	virtual bool run(Exception& ex, const volatile bool& requestStop) = 0;
	void		 process();

	static void  SetSystemName(const std::string& name);
	
	static thread_local std::string		_Name;
	static thread_local Thread*			_Me;

	const char*		_name;
	Priority		_priority;
	volatile bool	_stop;
	volatile bool	_requestStop;

	std::mutex		_mutex; // protect _thread
	std::thread		_thread;
};


} // namespace Base
