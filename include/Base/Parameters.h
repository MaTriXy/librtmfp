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
#include "Base/Event.h"

namespace Base {

struct Parameters : String::Object<Parameters> {

	typedef Event<void(const std::string& key, const std::string* pValue)> ON(Change);
	typedef Event<void()>												   ON(Clear);

	typedef std::map<std::string, std::string, String::IComparator>::const_iterator const_iterator;

private:
	struct ForEach {
		ForEach() : _begin(Null().end()), _end(Null().end()) {}
		ForEach(const_iterator begin, const_iterator end) : _begin(begin), _end(end) {}
		const_iterator		begin() const { return _begin; }
		const_iterator		end() const { return _end; }
	private:
		const_iterator  _begin;
		const_iterator  _end;
	};
public:

	Parameters() {}
	Parameters(Parameters&& other) { setParams(std::move(other));  }
	Parameters& setParams(Parameters&& other);

	const_iterator	begin() const { return _pMap ? _pMap->begin() : Null().begin(); }
	const_iterator	end() const { return _pMap ? _pMap->end() : Null().end(); }
	ForEach			from(const std::string& prefix) const { return _pMap ? ForEach(_pMap->lower_bound(prefix), _pMap->end()) : ForEach(); }
	ForEach			range(const std::string& prefix) const;
	UInt32			count() const { return _pMap ? _pMap->size() : 0; }

	Parameters&		clear(const std::string& prefix = String::Empty());

	/*!
	Return false if key doesn't exist (and don't change 'value'), otherwise return true and assign string 'value' */
	bool		getString(const std::string& key, std::string& value) const;
	/*!
	A short version of getString with default argument to get value by returned result */
	const char* getString(const std::string& key, const char* defaultValue = NULL) const;

	/*!
	Return false if key doesn't exist or if it's not a numeric type, otherwise return true and assign numeric 'value' */
	template<typename Type>
	bool getNumber(const std::string& key, Type& value) const { FATAL_ASSERT(std::is_arithmetic<Type>::value); const char* temp = getParameter(key); return temp && String::ToNumber(temp, value); }
	/*!
	A short version of getNumber with template default argument to get value by returned result */
	template<typename Type = double, int defaultValue = 0>
	Type getNumber(const std::string& key) const { FATAL_ASSERT(std::is_arithmetic<Type>::value); Type result((Type)defaultValue); getNumber(key, result); return result; }

	/*!
	Return false if key doesn't exist or if it's not a boolean type, otherwise return true and assign boolean 'value' */
	bool getBoolean(const std::string& key, bool& value) const;
	/*! A short version of getBoolean with template default argument to get value by returned result */
	template<bool defaultValue=false>
	bool getBoolean(const std::string& key) const { bool result(defaultValue); getBoolean(key, result); return result; }

	bool hasKey(const std::string& key) const { return getParameter(key) != NULL; }

	bool erase(const std::string& key);

	const std::string& setString(const std::string& key, const std::string& value) { return setParameter(key, value); }
	const std::string& setString(const std::string& key, const char* value, std::size_t size = std::string::npos) { return setParameter(key, value, size == std::string::npos ? strlen(value) : size); }

	template<typename Type>
	Type setNumber(const std::string& key, Type value) { FATAL_ASSERT(std::is_arithmetic<Type>::value); setParameter(key, String(value)); return value; }

	bool setBoolean(const std::string& key, bool value) { setParameter(key, value ? "true" : "false");  return value; }

	/*!
	Emplace key-value
	you can use it with String constructor to concat multiple chunk in key and value => emplace(String(pre, key), String(pre,value))
	or use it with piecewise_construct to prefer "multiple argument" to build string (to cut a string with size argument for example) => emplace(piecewise_construct, forward_as_tuple(key), std::forward_as_tuple(forward<Args>(args)...)) */
	template<typename ...Args>
	const std::string& emplace(Args&& ...args) {
		std::pair<std::string, std::string> item(std::forward<Args>(args)...);
		return setParameter(item.first, std::move(item.second));
	}

	static const Parameters& Null() { static Parameters Null(nullptr); return Null; }

protected:
	virtual void onParamChange(const std::string& key, const std::string* pValue) { onChange(key, pValue); }
	virtual void onParamClear() { onClear(); }

private:
	virtual const char* onParamUnfound(const std::string& key) const { return NULL; }

	Parameters(std::nullptr_t) : _pMap(new std::map<std::string, std::string, String::IComparator>()) {} // Null()

	const char* getParameter(const std::string& key) const;

	template<typename ...Args>
	const std::string& setParameter(const std::string& key, Args&& ...args) {
		if (!_pMap)
			_pMap.reset(new std::map<std::string, std::string, String::IComparator>());
		std::string value(std::forward<Args>(args)...);
		const auto& it = _pMap->emplace(key, std::string());
		if (it.second || it.first->second.compare(value) != 0) {
			it.first->second = std::move(value);
			onParamChange(key, &it.first->second);
		}
		return it.first->second;
	}

	// shared because a lot more faster than using st::map move constructor!
	// Also build _pMap just if required, and then not erase it but clear it (more faster that reset the shared)
	shared<std::map<std::string, std::string, String::IComparator>>	_pMap;
};



} // namespace Base

