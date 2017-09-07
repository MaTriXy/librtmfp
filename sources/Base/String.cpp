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

#include "Base/String.h"
#include "Base/Exceptions.h"
#include <limits>
#include <cctype>

using namespace std;

namespace Base {

#if defined(_WIN32) && (_MSC_VER < 1900)
	 // for setting number of exponent digits to 2
	static int output_exp_old_format(_set_output_format(_TWO_DIGIT_EXPONENT));
#endif

size_t String::Split(const char* value, size_t size, const char* separators, const String::ForEach& forEach, SPLIT_OPTIONS options) {
	const char* it1(value);
	const char* it2(NULL);
	const char* it3(NULL);
	size_t count(0);

	for(;;) {
		if (options & SPLIT_TRIM) {
			while ((*it1 && size) && isspace(*it1)) {
				++it1;
				--size;
			}
		}
		it2 = it1;
		while ((*it2 && size) && !strchr(separators, *it2)) {
			++it2;
			--size;
		}
		it3 = it2;
		if ((options & SPLIT_TRIM) && it3 != it1) {
			--it3;
			while (it3 != it1 && isspace(*it3))
				--it3;
			if (!isspace(*it3))
				++it3;
		}
		if (it3 != it1 || !(options&SPLIT_IGNORE_EMPTY)) {
			String::Scoped scoped(it3);
			if(!forEach(count++, &*it1))
				return string::npos;
		}
		it1 = it2;
		if (!*it1 || !size--)
			break;
		++it1;
	};
	return count;
}

vector<string>& String::Split(const char* value, size_t size, const char* separators, vector<string>& values, SPLIT_OPTIONS options) {
	ForEach forEach([&values](UInt32 index,const char* value){
		values.emplace_back(value);
		return true;
	});
	Split(value, size, separators, forEach, options);
	return values;
}

int String::ICompare(const char* value1, const char* value2,  size_t size) {
	if (value1 == value2)
		return 0;
	if (!value1)
		return -1;
	if (!value2)
		return 1;

	int f(0), l(0);
	do {
		if (size == 0)
			return f - l;
		if (((f = (unsigned char)(*(value1++))) >= 'A') && (f <= 'Z'))
			f -= 'A' - 'a';
		if (((l = (unsigned char)(*(value2++))) >= 'A') && (l <= 'Z'))
			l -= 'A' - 'a';
		if (size != std::string::npos)
			--size;
	} while (f && (f == l));

	return(f - l);
}

const char*	String::TrimLeft(const char* value, size_t size) {
	if (size == string::npos)
		size = strlen(value);
	while (size-- && isspace(*value))
		++value;
	return value;
}


size_t String::TrimRight(const char* value, size_t size) {
	const char* begin(value);
	if (size == string::npos)
		size = strlen(begin);
	value += size;
	while (value != begin && isspace(*--value))
		--size;
	return size;
}

template<typename Type, typename>
bool String::ToNumber(const char* value, size_t size, Type& result)  {
	Exception ex;
	return ToNumber<Type>(ex, value, size, result);
}

template<typename Type, typename>
bool String::ToNumber(Exception& ex, const char* value, size_t size, Type& result) {
	int comma = 0;	
	bool beginning = true, negative = false;

	long double number(0);

	bool isSigned = numeric_limits<Type>::is_signed;
	Type max = numeric_limits<Type>::max();

	const char* current(value);
	if (size == string::npos)
		size = strlen(value);

	while(current && size-->0) {

		if (iscntrl(*current) || *current==' ') {
			if (beginning) {
				++current;
				continue;
			}
			ex.set<Ex::Format>(value, " is not a correct number");
			return false;
		}

		if (*current == '-') {
			if (isSigned && beginning && !negative) {
				negative = true;
				++current;
				continue;
			}
			ex.set<Ex::Format>(value, " is not a correct number");
			return false;
		}

		if (*current == '.' || *current == ',') {
			if (comma == 0 && !beginning) {
				comma = 1;
				++current;
				continue;
			}
			ex.set<Ex::Format>(value, " is not a correct number");
			return false;
		}

		if (beginning)
			beginning = false;

		if (isdigit(*current) == 0) {
			ex.set<Ex::Format>(value, " is not a correct number");
			return false;
		}

		number = number * 10 + (*current - '0');
		comma *= 10;
		++current;
	}

	if (beginning) {
		ex.set<Ex::Format>("Empty string is not a number");
		return false;
	}

	if (comma > 0)
		number /= comma;

	if (number > max) {
		ex.set<Ex::Format>(value, " exceeds maximum number capacity");
		return false;
	}

	if (negative)
		number *= -1;

	result = (Type)number;
	return true;
}

#if defined(_WIN32)
const char* String::ToUTF8(const wchar_t* value,char buffer[PATH_MAX]) {
	WideCharToMultiByte(CP_UTF8, 0, value, -1, buffer, PATH_MAX, NULL, NULL);
	return buffer;
}
#endif


void String::ToUTF8(const char* value, size_t size, const String::OnEncoded& onEncoded) {
	const char* begin(value);
	char newValue[2];

	for(;;) {
		if (size != string::npos) {
			if (!size--)
				break;
		} else if (!*value)
			break;

		if (ToUTF8(*value, newValue)) {
			++value;
			continue;
		}

		if (value > begin)
			onEncoded(begin, value - begin);
		onEncoded(STR newValue, 2);

		begin = ++value;
	}

	if (value > begin)
		onEncoded(begin, value - begin);
}

bool String::ToUTF8(char value, char (&buffer)[2]) {
	if (value >=0)
		return true;
	buffer[0] = (unsigned char)(((value >> 6) & 0x1F) | 0xC0);
	buffer[1] = (unsigned char)((value & 0x3F) | 0x80);
	return false;
}

template bool  String::ToNumber(const char*, size_t, float&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, float&);
template bool  String::ToNumber(const char*, size_t, double&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, double&);
template bool  String::ToNumber(const char*, size_t, unsigned char&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, unsigned char&);
template bool  String::ToNumber(const char*, size_t, char&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, char&);
template bool  String::ToNumber(const char*, size_t, short&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, short&);
template bool  String::ToNumber(const char*, size_t, unsigned short&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, unsigned short&);
template bool  String::ToNumber(const char*, size_t, int&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, int&);
template bool  String::ToNumber(const char*, size_t, unsigned int&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, unsigned int&);
template bool  String::ToNumber(const char*, size_t, long&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, long&);
template bool  String::ToNumber(const char*, size_t, unsigned long&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, unsigned long&);
template bool  String::ToNumber(const char*, size_t, long long&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, long long&);
template bool  String::ToNumber(const char*, size_t, unsigned long long&);
template bool  String::ToNumber(Exception& ex, const char*, size_t, unsigned long long&);

} // namespace Base
