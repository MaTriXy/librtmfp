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

#include "Base/ThreadQueue.h"
#include "Base/Logs.h"


using namespace std;


namespace Base {

thread_local ThreadQueue* ThreadQueue::_PCurrent(NULL);

bool ThreadQueue::run(Exception&, const volatile bool& stopping) {
	_PCurrent = this;

	for (;;) {

		bool timeout = !wakeUp.wait(120000); // 2 mn of timeout
		
		for (;;) {
			shared<Runner> pRunner;
			{
				std::lock_guard<std::mutex> lock(_mutex);
				if(_runners.empty()) {
					if (timeout)
						stop(); // to set _stop immediatly!
					if (stopping)
						return true;
					break;
				}
				pRunner = move(_runners.front());
				_runners.pop_front();
			}

			Exception ex;
			setName(pRunner->name);
			AUTO_ERROR(pRunner->run(ex), pRunner->name);
		}
	}
	return true;
}

} // namespace Base
