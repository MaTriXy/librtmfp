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
#include "Base/Buffer.h"
#include "Base/Date.h"
#include <functional>
#include <vector>
#include <map>
#include <set>

#undef max

namespace Base {

#define CONST_STRING(STRING)	operator const std::string&() const { return STRING;} \
								std::size_t length() const { return (STRING).length(); } \
								bool	  empty() const { return (STRING).empty(); } \
								const char& operator[] (std::size_t pos) const { return (STRING)[pos]; } \
								const char& back() const { return (STRING).back(); } \
								const char& front() const { return (STRING).front(); } \
								const char* c_str() const { return (STRING).c_str(); }

struct Exception;

typedef UInt8 SPLIT_OPTIONS;
enum {
	SPLIT_IGNORE_EMPTY = 1, /// ignore empty tokens
	SPLIT_TRIM = 2,  /// remove leading and trailing whitespace from tokens
};


typedef UInt8 HEX_OPTIONS;
enum {
	HEX_CPP = 1,
	HEX_TRIM_LEFT = 2,
	HEX_UPPER_CASE = 4
};


/// Utility class for generation parse of strings
struct String : std::string, virtual Object {
	NULLABLE 

	/*!
	Object formatable, must be iterable with key/value convertible in string */
	template<typename Type>
	struct Object : virtual Base::Object {
		operator const Type&() const { return (const Type&)self; }
	protected:
		Object() { return; for (const auto& it : (const Type&)self) String(it.first, it.second); }; // to detect string iterability on build
	};

	template <typename ...Args>
	String(Args&&... args) {
		Assign<std::string>(*this, std::forward<Args>(args)...);
	}
	operator bool() const { return !empty(); }
	std::string& clear() { std::string::clear(); return *this; }

	static const std::string& Empty() { static std::string Empty; return Empty; }


	struct Comparator {
		bool operator()(const std::string& value1, const std::string& value2) const { return String::ICompare(value1, value2)<0; }
		bool operator()(const char* value1, const char* value2) const { return String::ICompare(value1, value2)<0; }
	};
	struct IComparator {
		bool operator()(const std::string& value1, const std::string& value2) const { return String::ICompare(value1, value2)<0; }
		bool operator()(const char* value1, const char* value2) const { return String::ICompare(value1, value2)<0; }
	};

	/*!
	Null terminate a string in a scoped place
	/!\ Can't work on a literal C++ declaration!
	/!\ When using by "data+size" way, address must be in data capacity! ( */
	struct Scoped {
		Scoped(const char* end) : _c((char&)*end), _cValue(*end) { _c = 0; }
		~Scoped() { _c = _cValue; }
	private:
		char& _c;
		char  _cValue;
	};

	/*!
	Encode value to UTF8 when required, if the value was already UTF8 compatible returns true, else false */
	static bool ToUTF8(char value, char (&buffer)[2]);
	/*!
	Encode value to UTF8, onEncoded returns concatenate piece of string encoded (to allow no data copy) */
	typedef std::function<void(const char* value, std::size_t size)> OnEncoded;
	static void ToUTF8(const char* value, const String::OnEncoded& onEncoded) { ToUTF8(value, std::string::npos, onEncoded); }
	static void ToUTF8(const std::string& value, const String::OnEncoded& onEncoded) { ToUTF8(value.data(), value.size(), onEncoded); }
	static void ToUTF8(const char* value, std::size_t size, const String::OnEncoded& onEncoded);
	
	typedef std::function<bool(UInt32 index,const char* value)> ForEach; /// String::Split function type handler
	static std::size_t Split(const std::string& value, const char* separators, const String::ForEach& forEach, SPLIT_OPTIONS options = 0) { return Split(value.data(), value.size(), separators, forEach, options); }
	static std::size_t Split(const char* value, const char* separators, const String::ForEach& forEach, SPLIT_OPTIONS options = 0) { return Split(value, std::string::npos, separators, forEach, options); }
	static std::size_t Split(const char* value, std::size_t size, const char* separators, const String::ForEach& forEach, SPLIT_OPTIONS options = 0);
	template<typename ListType, typename = typename std::enable_if<is_container<ListType>::value, ListType>::type>
	static ListType& Split(const char* value, const char* separators, ListType& values, SPLIT_OPTIONS options = 0) { return Split(value, std::string::npos, separators, values, options); }
	template<typename ListType, typename = typename std::enable_if<is_container<ListType>::value, ListType>::type>
	static ListType& Split(const std::string& value, const char* separators, ListType& values, SPLIT_OPTIONS options = 0) { return Split(value.data(), value.size(), separators,values,options); }
	template<typename ListType, typename = typename std::enable_if<is_container<ListType>::value, ListType>::type>
	static ListType& Split(const char* value, std::size_t size, const char* separators, ListType& values, SPLIT_OPTIONS options = 0) {
		ForEach forEach([&values](UInt32 index, const char* value) {
			values.insert(values.end(), value);
			return true;
		});
		Split(value, size, separators, forEach, options);
		return values;
	}

	template<typename Type>
	static const char*	TrimLeft(const char* value, Type& size) { static_assert(std::is_arithmetic<Type>::value, "size must be a numeric value"); if (size == std::string::npos) size = (Type)strlen(value);  while (size && isspace(*value)) { ++value; --size; } return value; }
	static const char*	TrimLeft(const char* value, std::size_t size = std::string::npos);
	template<typename Type>
	static char*		TrimRight(char* value, Type& size) { static_assert(std::is_arithmetic<Type>::value, "size must be a numeric value"); char* begin(value); if (size == std::string::npos) size = (Type)strlen(begin); value += size; while (value != begin && isspace(*--value)) --size; return begin; }
	static char*		TrimRight(char* value) { std::size_t size(strlen(value)); return TrimRight<std::size_t>(value, size); }
	static std::size_t	TrimRight(const char* value, std::size_t size = std::string::npos);
	template<typename Type>
	static char*		Trim(char* value, Type& size) { TrimLeft<Type>(value, size); return TrimRight<Type>(value, size); }
	static char*		Trim(char* value) { TrimLeft(value); return TrimRight(value); }
	static std::size_t	Trim(const char* value, std::size_t size = std::string::npos) { TrimLeft(value, size); return TrimRight(value, size); }

	static std::string&	TrimLeft(std::string& value) { return value.erase(0, TrimLeft(value.data(), value.size()) - value.data()); }
	static std::string&	TrimRight(std::string& value) { while (!value.empty() && isspace(value.back())) value.pop_back(); return value; }
	static std::string&	Trim(std::string& value) { TrimLeft(value); return TrimRight(value); }
	
	static std::string&	ToLower(std::string& value) { for (char& c : value) c = tolower(c); return value; }
	static std::string&	ToUpper(std::string& value) { for (char& c : value) c = toupper(c); return value; }

	static int ICompare(const char* value1, const char* value2,  std::size_t size = std::string::npos);
	static int ICompare(const std::string& value1, const std::string& value2, std::size_t size = std::string::npos) { return ICompare(value1.empty() ? NULL : value1.c_str(), value2.empty() ? NULL : value2.c_str(), size); }
	static int ICompare(const std::string& value1, const char* value2,  std::size_t size = std::string::npos) { return ICompare(value1.empty() ? NULL : value1.c_str(), value2, size); }
	static int ICompare(const char* value1, const std::string& value2,  std::size_t size = std::string::npos) { return ICompare(value1, value2.empty() ? NULL : value2.c_str(), size); }

	template<typename Type>
	static bool ToNumber(const std::string& value, Type& result, Math base = BASE_10) { return ToNumber(value.data(), value.size(), result, base); }
	template<typename Type>
	static bool ToNumber(const char* value, Type& result, Math base = BASE_10) { return ToNumber(value, std::string::npos, result, base); }
	template<typename Type>
	static bool ToNumber(const char* value, std::size_t size, Type& result, Math base = BASE_10);
	template<typename Type, long long defaultValue>
	static Type ToNumber(const std::string& value, Math base = BASE_10) { Type result; return ToNumber(value.data(), value.size(), result, base) ? result : defaultValue; }
	template<typename Type, long long defaultValue>
	static Type ToNumber(const char* value, Math base = BASE_10) { Type result; return ToNumber(value, std::string::npos, result, base) ? result : defaultValue; }
	template<typename Type, long long defaultValue>
	static Type ToNumber(const char* value, std::size_t size, Math base = BASE_10) { Type result; return ToNumber(value, size, result, base) ? result : defaultValue; }

	template<typename Type>
	static bool ToNumber(Exception& ex, const std::string& value, Type& result, Math base = BASE_10) { return ToNumber<Type>(ex, value.data(), value.size(), result, base); }
	template<typename Type>
	static bool ToNumber(Exception& ex, const char* value, Type& result, Math base = BASE_10) { return ToNumber<Type>(ex, value, std::string::npos, result, base); }
	template<typename Type>
	static bool ToNumber(Exception& ex, const char* value, std::size_t size, Type& result, Math base = BASE_10);
	template<typename Type, long long defaultValue>
	static Type ToNumber(Exception& ex, const std::string& value, Math base = BASE_10) { Type result;  return ToNumber(ex, value.data(), value.size(), result, base) ? result : defaultValue; }
	template<typename Type, long long defaultValue>
	static Type ToNumber(Exception& ex, const char* value, Math base = BASE_10) { Type result; return ToNumber(ex, value, std::string::npos, result, base) ? result : defaultValue; }
	template<typename Type, long long defaultValue>
	static Type ToNumber(Exception& ex, const char* value, std::size_t size, Math base = BASE_10) { Type result; return ToNumber(ex, value, size, result, base) ? result : defaultValue; }
	

	static bool IsTrue(const std::string& value) { return IsTrue(value.data(),value.size()); }
	static bool IsTrue(const char* value,std::size_t size=std::string::npos) { return ICompare(value, "1", size) == 0 || String::ICompare(value, "true", size) == 0 || String::ICompare(value, "yes", size) == 0 || String::ICompare(value, "on", size) == 0; }
	static bool IsFalse(const std::string& value) { return IsFalse(value.data(),value.size()); }
	static bool IsFalse(const char* value, std::size_t size = std::string::npos) { return ICompare(value, "0", size) == 0 || String::ICompare(value, "false", size) == 0 || String::ICompare(value, "no", size) == 0 || String::ICompare(value, "off", size) == 0 || String::ICompare(value, "null", size) == 0; }

	template <typename BufferType>
	static BufferType& ToHex(const std::string& value, BufferType& buffer) { return ToHex(value.c_str(), buffer); }
	template <typename BufferType>
	static BufferType& ToHex(const char* value, BufferType& buffer) {
		while (*value) {
			char left = toupper(*value++);
			char byte = *value ? toupper(*value++) : '0';
			byte = ((left - (left <= '9' ? '0' : '7')) << 4) | ((byte - (byte <= '9' ? '0' : '7')) & 0x0F);
			buffer.append(&byte, 1);
		}
		return buffer;
	}

	template<typename ValueType>
	struct Format : virtual Base::Object {
		Format(const char* format, const ValueType& value) : value(value), format(format) {}
		const ValueType&	value;
		const char*			format;
	};

	template <typename OutType, typename ...Args>
	static OutType& Assign(OutType& out, Args&&... args) {
		out.clear();
		return Append<OutType>(out, std::forward<Args>(args)...);
	}

	/// \brief match "std::string" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const std::string& value, Args&&... args) {
		return Append<OutType>((OutType&)out.append(value.data(), value.size()), std::forward<Args>(args) ...);
	}

	/// \brief match "const char*" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const char* value, Args&&... args) {
		return Append<OutType>((OutType&)out.append(value, strlen(value)), std::forward<Args>(args)...);
	}

	template <typename OutType, typename ...Args>
	static OutType& Append(String& out, std::string&& value, Args&&... args) { return Append<String>(out, value, std::forward<Args>(args) ...); }
	template <typename OutType, typename ...Args>
	static std::string& Append(std::string& out, std::string&& value, Args&&... args) {
		if (!out.size())
			out = std::move(value);
		else
			out.append(value.data(), value.size());
		return Append<std::string>(out, std::forward<Args>(args)...);
	}

	struct Lower : virtual Base::Object {
		Lower(const char* data, std::size_t size=std::string::npos) : data(data), size(size== std::string::npos ? std::strlen(data) : size) {}
		Lower(const std::string& data) : data(data.data()), size(data.size()) {}
		const char*			data;
		const std::size_t	size;
	};
	/// \brief match "const char*" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const Lower& value, Args&&... args) {
		for (std::size_t i = 0; i < value.size; ++i) {
			char c = tolower(value.data[i]);
			out.append(&c, 1);
		}
		return Append<OutType>(out, std::forward<Args>(args)...);
	}
	struct Upper : Lower { using Lower::Lower; };
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const Upper& value, Args&&... args) {
		for (std::size_t i = 0; i < value.size; ++i) {
			char c = toupper(value.data[i]);
			out.append(&c, 1);
		}
		return Append<OutType>(out, std::forward<Args>(args)...);
	}

#if defined(_WIN32)
	/// \brief match "wstring" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const std::wstring& value, Args&&... args) {
		return Append<OutType>(value.c_str(), std::forward<Args>(args)...);
	}
	
	/// \brief match "const wchar_t*" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const wchar_t* value, Args&&... args) {
		char buffer[PATH_MAX];
		ToUTF8(value, buffer);
		return Append<OutType>((OutType&)out.append(buffer, strlen(buffer)), std::forward<Args>(args)...);
	}
#endif

	// match le "char" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, char value, Args&&... args) {
		return Append<OutType>((OutType&)out.append(&value,1), std::forward<Args>(args)...);
	}

	// match le "signed char" cas
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, signed char value, Args&&... args) {
		char buffer[8];
		sprintf(buffer, "%hhd", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "short" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, short value, Args&&... args) {
		char buffer[8];
		sprintf(buffer, "%hd", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "int" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, int value, Args&&... args) {
		char buffer[16];
		sprintf(buffer, "%d", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "long" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, long value, Args&&... args) {
		char buffer[32];
		sprintf(buffer, "%ld", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "unsigned char" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, unsigned char value, Args&&... args) {
		char buffer[8];
		sprintf(buffer, "%hhu", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "unsigned short" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, unsigned short value, Args&&... args) {
		char buffer[8];
		sprintf(buffer, "%hu", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "unsigned int" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, unsigned int value, Args&&... args) {
		char buffer[16];
		sprintf(buffer, "%u", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "unsigned long" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, unsigned long value, Args&&... args) {
		char buffer[32];
		sprintf(buffer, "%lu", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "Int64" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, long long value, Args&&... args) {
		char buffer[64];
		sprintf(buffer, "%lld", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "UInt64" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, unsigned long long value, Args&&... args) {
		char buffer[64];
		sprintf(buffer, "%llu", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "float" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, float value, Args&&... args) {
		char buffer[64];
		sprintf(buffer, "%.8g", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "double" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, double value, Args&&... args) {
		char buffer[64];
		sprintf(buffer, "%.16g", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief match "bool" case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, bool value, Args&&... args) {
		if (value)
			return Append((OutType&)out.append(EXPAND("true")), std::forward<Args>(args)...);
		return Append<OutType>((OutType&)out.append(EXPAND("false")), std::forward<Args>(args)...);
	}

	/// \brief match pointer case
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const void* value, Args&&... args)	{
		char buffer[64];
		sprintf(buffer,"%p", value);
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	/// \brief A usefull form which use snprintf to format out
	///
	/// \param out This is the std::string which to append text
	/// \param value A pair of format text associate with value (ex: pair<char*, double>("%.2f", 10))
	/// \param args Other arguments to append
	template <typename OutType, typename Type, typename ...Args>
	static OutType& Append(OutType& out, const Format<Type>& custom, Args&&... args) {
		char buffer[64];
		try {
            snprintf(buffer, sizeof(buffer), custom.format, custom.value);
		}
		catch (...) {
			return Append<OutType>(out, std::forward<Args>(args)...);
		}
		return Append<OutType>((OutType&)out.append(buffer,strlen(buffer)), std::forward<Args>(args)...);
	}

	struct Data : virtual Base::Object {
		Data(const std::string& value, std::size_t size) : value(value.data()), size(size == std::string::npos ? value.size() : size) {}
		Data(const char* value,        std::size_t size) : value(value), size(size==std::string::npos ? strlen(value) : size) {}
		Data(const UInt8* value, UInt32 size) : value(STR value), size(size) {}
		const char*	value;
		UInt32 size;
	};
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const Data& data, Args&&... args) {
		return Append<OutType>((OutType&)out.append(data.value, data.size), std::forward<Args>(args)...);
	}

	struct Date : virtual Base::Object {
		Date(const Base::Date& date, const char* format) : format(format), _pDate(&date) {}
		Date(const char* format) : format(format), _pDate(NULL) {}
		const Base::Date*	operator->() const { return _pDate; }
		const char*			format;
	private:
		const Base::Date* _pDate;
	};
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const Date& date, Args&&... args) {
		const Base::Date* pDate = date.operator->();
		if(pDate)
			return Append<OutType>((OutType&)pDate->format(date.format, out), std::forward<Args>(args)...);
		return Append<OutType>((OutType&)Base::Date().format(date.format, out), std::forward<Args>(args)...);
	}


	struct Hex : virtual Base::Object {
		Hex(const UInt8* data, UInt32 size, HEX_OPTIONS options = 0) : data(data), size(size), options(options) {}
		const UInt8*		data;
		const UInt32		size;
		const HEX_OPTIONS	options;
	};
	template <typename OutType, typename ...Args>
	static OutType& Append(OutType& out, const Hex& hex, Args&&... args) {
		const UInt8 *end(hex.data + hex.size), *data(hex.data);
		bool skipLeft(false);
		if (hex.options&HEX_TRIM_LEFT) {
			while (data<end) {
				if (((*data) >> 4)>0)
					break;
				if (((*data) & 0x0F) > 0) {
					skipLeft = true;
					break;
				}
				++data;
			}
		}
		UInt8 ref(hex.options&HEX_UPPER_CASE ? '7' : 'W');
		UInt8 value;
		while (data<end) {
			if (hex.options&HEX_CPP)
				out.append(EXPAND("\\x"));
			value = (*data) >> 4;
			if (!skipLeft) {
				value += value > 9 ? ref : '0';
				out.append(STR &value, 1);
			} else
				skipLeft = false;
			value = (*data++) & 0x0F;
			value += value > 9 ? ref : '0';
			out.append(STR &value, 1);
		}
		return Append<OutType>(out, std::forward<Args>(args)...);
	}
	template <typename OutType, typename Type, typename ...Args>
	static OutType& Append(OutType& out, const Object<Type>& object, Args&&... args) {
		bool first = true;
		out.append(EXPAND("{"));
		for (const auto& it : (const Type&)object) {
			if (!first)
				out.append(EXPAND(", "));
			else
				first = false;
			Append<OutType>(out, it.first, ": ", it.second);
		}
		out.append(EXPAND("}"));
		return Append<OutType>(out, std::forward<Args>(args)...);
	}

	template <typename OutType>
	static OutType& Append(OutType& out) { return out; }

private:

#if defined(_WIN32)
	static const char* ToUTF8(const wchar_t* value, char buffer[PATH_MAX]);
#endif
};

inline bool operator==( const std::string& left, const char* right) { return left.compare(right) == 0; }
inline bool operator==(const std::string& left, const std::string& right) { return left.compare(right) == 0; }
inline bool operator==(const char* left, const std::string& right) { return right.compare(left) == 0; }
inline bool operator!=(const std::string& left, const char* right) { return left.compare(right) != 0; }
inline bool operator!=(const std::string& left, const std::string& right) { return left.compare(right) != 0; }
inline bool operator!=(const char* left, const std::string& right) { return right.compare(left) != 0; }
inline bool operator<(const std::string& left, const char* right) { return left.compare(right) < 0; }
inline bool operator<(const std::string& left, const std::string& right) { return left.compare(right) < 0; }
inline bool operator<(const char* left, const std::string& right) { return right.compare(left) > 0; }
inline bool operator<=(const std::string& left, const char* right) { return left.compare(right) <= 0; }
inline bool operator<=(const std::string& left, const std::string& right) { return left.compare(right) <= 0; }
inline bool operator<=(const char* left, const std::string& right) { return right.compare(left) >= 0; }
inline bool operator>(const std::string& left, const char* right) { return left.compare(right) > 0; }
inline bool operator>(const std::string& left, const std::string& right) { return left.compare(right) > 0; }
inline bool operator>(const char* left, const std::string& right) { return right.compare(left) < 0; }
inline bool operator>=(const std::string& left, const char* right) { return left.compare(right) >= 0; }
inline bool operator>=(const std::string& left, const std::string& right) { return left.compare(right) >= 0; }
inline bool operator>=(const char* left, const std::string& right) { return right.compare(left) <= 0; }


} // namespace Base

