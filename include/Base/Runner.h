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

namespace Base {


struct Runner : virtual Object {
	Runner(const std::string& name) : name(name) {}

	const std::string name;

	// If ex is raised, an error is displayed if the operation has returned false
	// otherwise a warning is displayed
	virtual bool run(Exception& ex) = 0;
};


} // namespace Base
