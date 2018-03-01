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


#include <stdio.h>
#include <cstdlib>
#include <cstdint>
#include <string>
#include <cstring>
#include <limits>
#include <memory>
#include <functional>
#include <cmath> // C++ version of math.h to solve abs ambiguity over linux


/////  Usefull macros and patchs   //////

#define self    (*this)
#define NULLABLE explicit operator void() { static_assert(std::is_constructible<bool, decltype(*this)>::value || std::is_convertible<decltype(*this), bool>::value, "Missing nullable operator"); }

#define BIN		(Base::UInt8*)
#define STR		(char*)

#define EXPAND(VALUE)	VALUE"",(sizeof(VALUE)-1) // "" concatenation is here to check that it's a valid const string is not a pointer of char*

#define STRINGIZE(x) STRINGIZE2(x)
#define STRINGIZE2(x) #x
#define LINE_STRING STRINGIZE(__LINE__)

#if defined(_WIN32)
#define _WINSOCKAPI_    // stops windows.h including winsock.h
#define NOMINMAX
#include "windows.h"
#define sprintf sprintf_s
#define snprintf sprintf_s
#define PATH_MAX 4096 // to match Linux!
#define __BIG_ENDIAN__ 0 // windows is always little endian!
#elif defined(__FreeBSD__) || defined(__APPLE__) || defined(__TOS_MACOS__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define _BSD 1 // Detect BSD system
#endif


//
// Automatically link Base library.
//
#if defined(_MSC_VER)
// disable the "throw noexception" warning because Mona has its own exception and can use everywhere std throw on FATAL problem (unexpected behavior)
#pragma warning(disable: 4297)
#if defined(_DEBUG)
	#pragma comment(lib, "libeayd.lib")
	#pragma comment(lib, "ssleayd.lib")
#else
	#pragma comment(lib, "libeay.lib")
	#pragma comment(lib, "ssleay.lib")
#endif
#endif


namespace Base {


#if defined(_DEBUG) && defined(_WIN32)
#define new new(_NORMAL_BLOCK, __FILE__, __LINE__)
void DetectMemoryLeak();
#else
void DetectMemoryLeak();
#endif



///// TYPES /////
template<typename Type>
using shared = std::shared_ptr<Type>;
template<typename Type>
using weak = std::weak_ptr<Type>;
template<typename Type>
using unique = std::unique_ptr<Type>;


typedef int8_t			Int8;
typedef uint8_t			UInt8;
typedef int16_t			Int16;
typedef uint16_t		UInt16;
typedef int32_t         Int32;
typedef uint32_t        UInt32;
typedef int64_t			Int64;
typedef uint64_t		UInt64;



//////  No copy, no move, objet nullable  //////


struct Static {
private:
	Static() = delete;
	Static(const Static& other) = delete;
	Static& operator=(const Static& other) = delete;
	Static(Static&& other) = delete;
	Static& operator=(Static&& other) = delete;
};
 
struct Object {
	Object() {}
	virtual ~Object() {};
private:
	Object(const Object& other) = delete;
	Object& operator=(const Object& other) = delete;
	Object(Object&& other) = delete;
	Object& operator=(Object&& other) = delete;
};

////// ASCII ////////

struct ASCII : virtual Static {
	enum Type {
		CONTROL  = 0x0001,
		BLANK    = 0x0002,
		SPACE    = 0x0004,
		PUNCT    = 0x0008,
		DIGIT    = 0x0010,
		HEXDIGIT = 0x0020,
		ALPHA    = 0x0040,
		LOWER    = 0x0080,
		UPPER    = 0x0100,
		GRAPH    = 0x0200,
		PRINT    = 0x0400,
		XML		 = 0x0800
	};

	static UInt8 ToLower(char value) { return Is(value, UPPER) ? (value + 32) : value; }
	static UInt8 ToUpper(char value) { return Is(value, LOWER) ? (value - 32) : value; }

	static bool Is(char value,UInt16 type) {return value&0x80 ? 0 : ((_CharacterTypes[int(value)]&type) != 0);}
private:
	static const UInt16 _CharacterTypes[128];
};

inline UInt64 abs(double value) { return (UInt64)std::abs(value); }
inline UInt64 abs(float value) { return (UInt64)std::abs(value); }
inline UInt64 abs(Int64 value) { return (UInt64)std::abs(value); }
inline UInt32 abs(Int32 value) { return (UInt32)std::abs(value); }
inline UInt16 abs(Int16 value) { return (UInt16)std::abs(value); }
inline UInt8 abs(Int8 value) { return (UInt8)std::abs(value); }

template<typename Type1, typename Type2, typename ResultType = typename std::make_signed<typename std::conditional<sizeof(Type1) >= sizeof(Type2), Type1, Type2>::type>::type>
inline ResultType distance(Type1 value1, Type2 value2) {
	ResultType result(value2 - value1);
	return abs(result) > (std::numeric_limits<typename std::make_unsigned<ResultType>::type>::max() >> 1) ? (value1 - value2) : result;
}

inline bool isalnum(char value) { return ASCII::Is(value, ASCII::ALPHA | ASCII::DIGIT); }
inline bool isalpha(char value) { return ASCII::Is(value,ASCII::ALPHA); }
inline bool isblank(char value) { return ASCII::Is(value,ASCII::BLANK); }
inline bool iscntrl(char value) { return ASCII::Is(value,ASCII::CONTROL); }
inline bool isdigit(char value) { return ASCII::Is(value,ASCII::DIGIT); }
inline bool isgraph(char value) { return ASCII::Is(value,ASCII::GRAPH); }
inline bool islower(char value) { return ASCII::Is(value,ASCII::LOWER); }
inline bool isprint(char value) { return ASCII::Is(value,ASCII::PRINT); }
inline bool ispunct(char value) { return ASCII::Is(value,ASCII::PUNCT); }
inline bool isspace(char value) { return  ASCII::Is(value,ASCII::SPACE); }
inline bool isupper(char value) { return ASCII::Is(value,ASCII::UPPER); }
inline bool isxdigit(char value) { return ASCII::Is(value,ASCII::HEXDIGIT); }
inline bool isxml(char value) { return ASCII::Is(value,ASCII::XML); }
inline char tolower(char value) { return ASCII::ToLower(value); }
inline char toupper(char value) { return ASCII::ToUpper(value); }


const char* strrpbrk(const char* value, const char* markers);
const char *strrstr(const char* where, const char* what);

template<typename Type>
inline Type min(Type value) { return value; }
template<typename Type1, typename Type2, typename ...Args>
inline typename std::conditional<sizeof(Type1) >= sizeof(Type2), Type1, Type2>::type
				  min(Type1 value1, Type2 value2, Args&&... args) { return value1 < value2 ? min(value1, args ...) : min(value2, args ...); }

template<typename Type>
inline Type max(Type value) { return value; }
template<typename Type1, typename Type2, typename ...Args>
inline typename std::conditional<sizeof(Type1) >= sizeof(Type2), Type1, Type2>::type
				  max(Type1 value1, Type2 value2, Args&&... args) { return value1 > value2 ? max(value1, args ...) : max(value2, args ...); }

template<typename RangeType, typename Type>
inline RangeType range(Type value) { return value > std::numeric_limits<RangeType>::max() ? std::numeric_limits<RangeType>::max() : ((std::is_signed<Type>::value && value < std::numeric_limits<RangeType>::min()) ? std::numeric_limits<RangeType>::min() : (RangeType)value); }

const std::string& typeof(const std::type_info& info);
template<typename ObjectType>
inline const std::string& typeof(const ObjectType& object) { return typeof(typeid(object)); }
/*!
Try to prefer this template typeof version, because is the more faster*/
template<typename ObjectType>
inline const std::string& typeof() {
	static struct Type : std::string { Type() : std::string(typeof(typeid(ObjectType))) {} } Type;
	return Type;
}

template<typename MapType>
inline typename MapType::iterator lower_bound(MapType& map, const typename MapType::key_type& key, const std::function<bool(const typename MapType::key_type&, typename MapType::iterator&)>& validate) {
	typename MapType::iterator it, result(map.begin());
	typename MapType::key_compare less = map.key_comp();
	UInt32 count(map.size()), step;
	while (count) {
		if (!validate(key, result)) {
			result = map.erase(result);
			if (!--count)
				return result;
		}
		it = result;
		step = count / 2;
		std::advance(it, step);
		if (less(it->first, key)) {
			result = ++it;
			count -= step + 1;
		} else
			count = step;
	}
	return result;
}

template <typename Type>
class is_container : virtual Static {
	template<typename C> static char(&B(typename std::enable_if<
		std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::begin)), typename C::const_iterator(C::*)() const>::value,
		void
	>::type*))[1];
	template<typename C> static char(&B(...))[2];

	template<typename C> static char(&E(typename std::enable_if<
		std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::end)), typename C::const_iterator(C::*)() const>::value,
		void
	>::type*))[1];
	template<typename C> static char(&E(...))[2];

	template<typename C> static char(&I(typename std::enable_if<
		std::is_same<decltype(static_cast<typename C::iterator(C::*)(typename C::const_iterator, typename C::value_type&&)>(&C::insert)), typename C::iterator(C::*)(typename C::const_iterator, typename C::value_type&&)>::value,
		void
	>::type*))[1];
	template<typename C> static char(&I(...))[2];
public:
	static bool const value = sizeof(B<Type>(0)) == 1 && sizeof(E<Type>(0)) == 1 && sizeof(I<Type>(0)) == 1;
};

/*!
enum mathematics constant (base and others) */
enum Math {
	BASE_1 = 1,
	BASE_2 = 2,
	BASE_3 = 3,
	BASE_4 = 4,
	BASE_5 = 5,
	BASE_6 = 6,
	BASE_7 = 7,
	BASE_8 = 8,
	BASE_9 = 9,
	BASE_10 = 10,
	BASE_11 = 11,
	BASE_12 = 12,
	BASE_13 = 13,
	BASE_14 = 14,
	BASE_15 = 15,
	BASE_16 = 16,
	BASE_17 = 17,
	BASE_18 = 18,
	BASE_19 = 19,
	BASE_20 = 20,
	BASE_21 = 21,
	BASE_22 = 22,
	BASE_23 = 23,
	BASE_24 = 24,
	BASE_25 = 25,
	BASE_26 = 26,
	BASE_27 = 27,
	BASE_28 = 28,
	BASE_29 = 29,
	BASE_30 = 30,
	BASE_31 = 31,
	BASE_32 = 32,
	BASE_33 = 33,
	BASE_34 = 34,
	BASE_35 = 35,
	BASE_36 = 36
};



} // namespace Base
