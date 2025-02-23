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

#include "Base/Date.h"
#include "Base/Exceptions.h"


using namespace std;

namespace Base {


const char* Date::FORMAT_ISO8601("%Y[-%m-%dT%H:%M:%S%z]");  	  // 2005-01-01T12:00:00+01:00 | 2005-01-01T11:00:00Z
const char* Date::FORMAT_ISO8601_FRAC("%Y[-%m-%dT%H:%M:%s%z]");   // 2005-01-01T12:00:00.000000+01:00 | 2005-01-01T11:00:00.000000Z
const char* Date::FORMAT_ISO8601_SHORT("%Y[%m%dT%H%M%S%z]");  	  // 20050101T120000+01:00 | 20050101T110000Z
const char* Date::FORMAT_ISO8601_SHORT_FRAC("%Y[%m%dT%H%M%s%z]"); // 20050101T120000.000000+01:00 | 20050101T110000.000000Z
const char* Date::FORMAT_RFC822("[%w, ]%e %b %y %H:%M[:%S] %Z");  // Sat, 1 Jan 05 12:00:00 +0100 | Sat, 1 Jan 05 11:00:00 GMT
const char* Date::FORMAT_RFC1123("%w, %e %b %Y %H:%M:%S %Z");	  // Sat, 1 Jan 2005 12:00:00 +0100 | Sat, 1 Jan 2005 11:00:00 GMT
const char* Date::FORMAT_HTTP("%w, %d %b %Y %H:%M:%S %Z");		  // Sat, 01 Jan 2005 12:00:00 +0100 | Sat, 01 Jan 2005 11:00:00 GMT
const char* Date::FORMAT_RFC850("%W, %e-%b-%y %H:%M:%S %Z");	  // Saturday, 1-Jan-05 12:00:00 +0100 | Saturday, 1-Jan-05 11:00:00 GMT
const char* Date::FORMAT_RFC1036("%W, %e %b %y %H:%M:%S %Z");	  // Saturday, 1 Jan 05 12:00:00 +0100 | Saturday, 1 Jan 05 11:00:00 GMT
const char* Date::FORMAT_ASCTIME("%w %b %f %H:%M:%S %Y");		  // Sat Jan  1 12:00:00 2005
const char* Date::FORMAT_SORTABLE("%Y-%m-%d[ %H:%M:%S]");		  // 2005-01-01 12:00:00



const char* Date::_WeekDayNames[] = {
	"Sunday",
	"Monday",
	"Tuesday",
	"Wednesday",
	"Thursday",
	"Friday",
	"Saturday"
};

const char* Date::_MonthNames[] = {
	"January",
	"February",
	"March",
	"April",
	"May",
	"June",
	"July",
	"August",
	"September",
	"October",
	"November",
	"December"
};

static const UInt16 _MonthDays[][12] = {
	{ 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334 },
	{ 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335} // leap
};

static Int32 LeapYears(Int32 year) {
	Int32 result(year/4); // every 4 year
	result -= year/100; // but not divisible by 100
	result += year/400; // except divisible by 400
	result -= 477; // 1970 reference
	if (year <= 0) // cause 0 is not leap, and it's delayed in the negative: -1, -5, etc..
		--result;
	return result;
}

Date& Date::update(Int64 time,Int32 offset) {
	Time::update(time);
	if (_day == 0) {
		_offset = offset;
		return *this;
	}

	_changed = false;

	// update time with offset
	_offset = offset;
	_isLocal = _isDST = false;
	if (_offset == Timezone::GMT)
		_offset = 0;
	else if (_offset == Timezone::LOCAL) {
		_offset = Timezone::GMT;
		_offset = Timezone::Offset(*this,_isDST);
		_isLocal = true;
	}
	time += _offset;

	Int64 days((Int64)floor(time / 86400000.0));

	// in first, compute weak day
	computeWeekDay(days);

	if (time >= 0)
		++days;
	Int32 year = _year = 0;
	Int32 delta(0);
	while ((year = (Int32)(days / 366)) != _year) {
		days -= delta;
		delta = year-LeapYears(year + 1970);
		days += delta;
		_year=year;
	}
	days %= 366;
	year = _year += 1970;
	bool isLeap(IsLeapYear(_year));
	if (time < 0) {
		if (_year>0 && isLeap)
			++days;
		if (days < 0)
			--_year;
		++days;
	}
	if (_year!=year)
		isLeap = IsLeapYear(_year);

	if (time >= 0) {
		if (isLeap)
			++days;
	} else if (days <= 0)
		days += (isLeap ? 366 : 365);

	_month = 1;
	UInt16 count(0);
	while (_month<12 && days > (delta=_MonthDays[isLeap ? 1 : 0][_month])) {
		++_month;
		count = delta;
	}
	days -= count;

	_day = (UInt8)days;
	
	time = time%86400000;
	if (time < 0)
		time += 86400000;

	_hour = (UInt8)(time / 3600000);
	time %= 3600000;
	_minute = (UInt8)(time / 60000);
	time %= 60000;
	_second = (UInt8)(time / 1000);
	time %= 1000;

	_millisecond = (UInt16)time;
	return *this;
}

Int64 Date::time() const {
	if (!_changed) // if _day==0 then _changed==false!
		return Time::time();

	Int64 time = _day - 1 + LeapYears(_year);
	bool isLeap(IsLeapYear(_year));
	if (isLeap && _year > 0)
		--time;
	 time += _MonthDays[isLeap?1:0][_month-1];

	 time += (_year - 1970)*365;
	 UInt32 clock(this->clock());
	 time = time * 86400000 + clock;

	_changed = false;
	_weekDay = 7;
	if (_offset == Timezone::GMT) {
		_isLocal = _isDST = false;
		_offset = 0;
	} else if (_offset == Timezone::LOCAL || _isLocal) {
		_offset = Timezone::GMT;
		((Date*)this)->Time::update(time);
		_offset = Timezone::Offset(*this, _isDST); // will call time(), offset() and weekDay()
		_isLocal = true;
	} else
		_isLocal = false;
	// time is expressed in local, change it for GMT
	return ((Date&)*this).Time::update(time-_offset);
}

Int32 Date::offset() const {
	if (_day == 0) {
		init();
		return _offset;
	}

	if (_offset == Timezone::GMT) {
		_offset = 0;
		_isDST = _isLocal = false;
		return 0;
	}
	if (_offset != Timezone::LOCAL && !_isLocal)
		return _offset;  // _offset is a fix value

	if (_changed)
		time();  // assign _isLocal, _offset and _isDST
	else if (_offset == Timezone::LOCAL) {
		_offset = Timezone::GMT;
		_offset = Timezone::Offset(*this, _isDST);
		_isLocal = true;
	}
	
	return _offset;
}

void Date::setOffset(Int32 offset) {
	if (_day == 0 || _changed) {
		_offset = offset;
		return;
	}

	// here _offset is a fixed value because _day!=0 and !_changed
	if (offset == Timezone::LOCAL) {
		if (_isLocal)
			return;
		_isLocal = true;
		offset = Timezone::Offset(_isDST);
	} else if (offset == Timezone::GMT) {
		_isDST = _isLocal = false;
		if (_offset==0)
			return;
		offset = 0;
	}
	
	// now both (offset and _offset) are fixed value
	Time::update(Time::time() + _offset - offset); // time + old - new
	_offset = offset;
}



Date& Date::update(const Date& date) {
	_year = date._year;
	_month = date._month;
	_day = date._day;
	_weekDay = date._weekDay;
	_hour = date._hour;
	_minute = date._minute;
	_second = date._second;
	_millisecond = date._millisecond;
	_offset = date._offset;
	_isLocal = date._isLocal;
	_isDST = date._isDST;
	Time::update((Time&)date);
	return *this;
}

Date& Date::update(Int32 year, UInt8 month, UInt8 day, UInt8 hour, UInt8 minute, UInt8 second, UInt16 millisecond) {
	setYear(year);
	setMonth(month);
	setClock(hour, minute, second, millisecond);
	// keep the following line in last position
	setDay(day);
	return *this;
}


Date& Date::update(Int32 year, UInt8 month, UInt8 day) {
	setYear(year);
	setMonth(month);
	// keep the following line in last position
	setDay(day);
	return *this;
}

void Date::setClock(UInt32 clock) {
	setHour(UInt8(clock / 3600000));
	clock %= 3600000;
	setMinute(UInt8(clock / 60000));
	clock %= 60000;
	setSecond(UInt8(clock / 1000));
	setMillisecond(clock%1000);
}

void Date::setClock(UInt8 hour, UInt8 minute, UInt8 second, UInt16 millisecond) {
	setHour(hour);
	setMinute(minute);
	setSecond(second);
	setMillisecond(millisecond);
}

void Date::setYear(Int32 year) {
	if (_day == 0)
		init();
	if (year == _year)
		return;
	_changed = true;
	_year = year;
}

void Date::setMonth(UInt8 month) {
	if (month<1)
		month = 1;
	else if (month>12)
		month = 12;
	if (_day == 0)
		init();
	if (month == _month)
		return;
	_changed = true;
	_month = month;
}

void Date::setWeekDay(UInt8 weekDay) {
	weekDay %= 7;
	Int8 delta(weekDay - this->weekDay());
	if (delta==0)
		return;

	setDay(day()+delta);
	if (this->weekDay() == weekDay)
		return;
	if (delta>0)
		delta -= 7;
	else
		delta += 7;
	setDay(day()+delta);
}

void Date::setYearDay(UInt16 yearDay) {
	if (_day == 0)
		init(); // to get _year

	bool isLeap(IsLeapYear(_year));
	UInt8 month(1);
	while (month<12 && yearDay > _MonthDays[isLeap ? 1 : 0][month])
		++month;
	setMonth(month);
	setDay(yearDay - _MonthDays[isLeap ? 1 : 0][month-1]);
}

void Date::setDay(UInt8 day) {
	if (_day == 0)
		init();
	if (day == _day)
		return;

	if (day<1)
		day = 1;
	else if (day>31)
		day = 31;
	if (day > 28) {
		if (_month < 8) {
			if (_month == 2) {
				if (day >= 30)
					day = 29;
				if (day == 29 && !IsLeapYear(_year))
					day = 28;
			} else if (day==31 && !(_month & 0x01))
				day = 30;
		} else if(day==31) {
			if (_month & 0x01)
				day = 30;
		}
	}
	
	if (day == _day)
		return;
	_changed = true;
	_day = day;
}

void Date::setHour(UInt8 hour) {
	if (_day == 0)
		init();
	if (hour > 59)
		hour = 59;
	if (hour == _hour)
		return;
	_changed = true;
	_hour = hour;
}

void Date::setMinute(UInt8 minute) {
	if (_day == 0)
		init();
	if (minute>59)
		minute = 59;
	if (minute == _minute)
		return;
	_changed = true;
	_minute = minute;
}

void Date::setSecond(UInt8 second) {
	if (_day == 0)
		init();
	if (second>59)
		second = 59;
	if (second == _second)
		return;
	_changed = true;
	_second = second;
}

void Date::setMillisecond(UInt16 millisecond) {
	if (_day == 0)
		init();
	if (millisecond > 999)
		millisecond = 999;
	if (millisecond == _millisecond)
		return;
	if (!_changed)
		Time::update(time()-_millisecond+millisecond);
	_millisecond = millisecond;
}

UInt8 Date::weekDay() const {
	if (_day == 0)
		init(); // will assign _weekDay
	else if (_changed || _weekDay==7)
		((Date&)*this).computeWeekDay((Int64)floor((time()+offset()) / 86400000.0));
	return _weekDay;
}

UInt16 Date::yearDay() const {
	// 0 to 365
	if (_day == 0)
		init();
	return _day+ _MonthDays[IsLeapYear(_year) ? 1 : 0][_month]-1;
}

void Date::computeWeekDay(Int64 days) {
	Int64 result(days += 4);
	if (days < 0)
		++result;
	result %= 7;
	if (days<0)
		result += 6;
	_weekDay = (UInt8)result;
}



///////////// PARSER //////////////////////////



#define CAN_READ (*current && size!=0)
#define READ	 (--size,*current++)

#define SKIP_DIGITS \
	while (CAN_READ && isdigit(*current)) READ;

#define PARSE_NUMBER(var) \
	while (CAN_READ && isdigit(*current)) var = var * 10 + (READ - '0')

#define PARSE_NUMBER_N(var, n) \
	{ size_t i = 0; while (i++ < n && CAN_READ && isdigit(*current)) var = var * 10 + (READ - '0');}

#define PARSE_FRACTIONAL_N(var, n) \
	{ size_t i = 0; while (i < n && CAN_READ && isdigit(*current)) { var = var * 10 + (READ - '0'); i++; } while (i++ < n) var *= 10; }





bool Date::update(Exception& ex, const char* current, size_t size, const char* format) {
	if (!format)
		return parseAuto(ex, current,size);

	UInt8 month(0), day(0), hour(0), minute(0), second(0);
	Int32 year(0), offset(Timezone::LOCAL);
	UInt16 millisecond(0);
	int microsecond(0);
	bool isDST(false);
	int optional(0);
	Int64 _time = 0;

	while (*format) {

		char c(*(format++));

		if (c == '[') {
			if (optional>=0)
				++optional;
			else
				--optional;
			continue;
		}
		if (c == ']') {
			if (optional > 0)
				--optional;
			else
				++optional;
			continue;
		}
		if (c != '%') {
			if (c == '?') {} 
			else if (optional<0)
				continue;
			else if (optional > 0) {
				if (!CAN_READ || c != *current) {
					optional = -optional;
					continue;
				}
			} else if (!CAN_READ || c != *current) {
				ex.set<Ex::Format>(*current, " doesn't match with ", c);
				return false;
			}
			READ;
			continue;
		}
		
		c = *format;
		if (!c)
			break;

		switch (*format++) {
			default:
				if (!optional) {
					ex.set<Ex::Format>("Unknown date ", c, " pattern");
					return false;
				}
				break;
			case '%': // Allow % in the string in catching %% case
				if (*current != '%') {
					ex.set<Ex::Format>("% doesn't match with ", *current);
					return false;
				}
				break;
			case 'w':
			case 'W':
				while (CAN_READ && isalpha(*current))
					READ;
				break;
			case 'b':
			case 'B': {
				month = 0;
				const char* value(current);
				while (CAN_READ && isalpha(*current))
					READ;
				if (current-value >= 3) {
					for (int i = 0; i < 12; ++i) {
						if (String::ICompare(_MonthNames[i], value, current - value) == 0) {
							month = i + 1;
							break;
						}
					}
				}
				if (!month && !optional) {
					ex.set<Ex::Format>("Impossible to parse ", value, " as a valid month");
					return false;
				}
				break;
			}
			case 'd':
			case 'e':
			case 'f':
				if (CAN_READ && isspace(*current))
					READ;
				PARSE_NUMBER_N(day, 2);
				break;
			case 'm':
			case 'n':
			case 'o':
				PARSE_NUMBER_N(month, 2);
				break;
			case 'y':
				PARSE_NUMBER_N(year, 2);
				if (year >= 70)
					year += 1900;
				else
					year += 2000;
				break;
			case 'Y':
				PARSE_NUMBER_N(year, 4);
				break;
			case '_':
				PARSE_NUMBER(year);
				if (year < 100) {
					if (year >= 70)
						year += 1900;
					else
						year += 2000;
				}
				break;
			case 'H':
			case 'h':
				PARSE_NUMBER_N(hour, 2);
				break;
			case 'T': {
				UInt32 factor(1);
				if (isalpha(*format)) {
					switch (tolower(*format++)) {
						case 'h':
							factor = 3600000;
							break;
						case 'm':
							factor = 60000;
							break;
						case 's':
							factor = 1000;
							break;
					}
				}
				const char* times = current;
				UInt32 count(0);
				while (CAN_READ && isdigit(*current)) {
					READ;
					++count;
				}
				if (!count && !optional) {
					ex.set<Ex::Format>("No time value to parse");
					return false;
				}
				_time += String::ToNumber<Int64, 0>(times, count) * factor;
				break;
			}
			case 'a':
			case 'A': {
				const char* ampm(current);
				UInt32 count(0);
				while (CAN_READ && isalpha(*current)) {
					READ;
					++count;
				}
				if (String::ICompare(ampm, count, "AM")==0) {
					if (hour == 12)
						hour = 0;
				} else if (String::ICompare(ampm, count, "PM")==0) {
					if (hour < 12)
						hour += 12;
				} else if (!optional) {
					ex.set<Ex::Format>("Impossible to parse ", ampm, " as a valid AM/PM information");
					return false;
				}
				break;
			}
			case 'M':
				PARSE_NUMBER_N(minute, 2);
				break;
			case 'S':
				PARSE_NUMBER_N(second, 2);
				break;
			case 's':
				PARSE_NUMBER_N(second, 2);
				if (CAN_READ && (*current == '.' || *current == ',')) {
					READ;
					PARSE_FRACTIONAL_N(millisecond, 3);
					PARSE_FRACTIONAL_N(microsecond, 3);
					SKIP_DIGITS;
				}
				break;
			case 'i':
				PARSE_NUMBER_N(millisecond, 3);
				break;
			case 'c':
				PARSE_NUMBER_N(millisecond, 1);
				millisecond *= 100;
				break;
			case 'F':
				PARSE_FRACTIONAL_N(millisecond, 3);
				PARSE_FRACTIONAL_N(microsecond, 3);
				SKIP_DIGITS;
				break;
			case 'z':
			case 'Z':

				offset = Timezone::LOCAL;
				const char* code(current);
				UInt32 count(0);
				while (CAN_READ && isalpha(*current)) {
					READ;
					++count;
				}
				if (count>0) {
					if (*current)
						offset = Timezone::Offset(string(code,count),isDST);
					else
						offset = Timezone::Offset(code,isDST);
				}
				if (CAN_READ && (*current == '+' || *current == '-')) {
					if (offset== Timezone::GMT || offset== Timezone::LOCAL)
						offset = 0;

					int sign = READ == '+' ? 1 : -1;
					int hours = 0;
					PARSE_NUMBER_N(hours, 2);
					if (CAN_READ && *current == ':')
						READ;
					int minutes = 0;
					PARSE_NUMBER_N(minutes, 2);
					offset += sign*(hours * 3600 + minutes * 60)*1000;
				}			
				break;
		}
	}

	update(year,month,day,hour,minute,second,millisecond,offset);
	_isDST = isDST;

	if (_time)
		update(time() + _time);

	if (microsecond > 0)
		ex.set<Ex::Format>("Microseconds information lost, not supported by Mona Date system");
	return true;
}

bool Date::parseAuto(Exception& ex, const char* data, size_t count) {

	size_t length(0),tPos(0);
	bool digit(false);
	bool digits(false);
	const char* current = data;
	size_t size(count);

	while(CAN_READ && length<50) {
		char c(*current);
		if (digit && c == 'T')
			tPos = length;
		if (length<10) {
			if (length == 0)
				digit = isdigit(c) != 0;
			else if (length <= 2) {
				if (length == 1) { 
					if (digit) {
						digits = true;
						if (!isdigit(c))
							return update(ex, data, count, "%e?%b?%_ %H:%M[:%S %Z]");
					}
				} else if (digits && !isdigit(c))
					return update(ex, data, count, "%e?%b?%_ %H:%M[:%S %Z]");
			} else if (length==3 && c==' ')
				return update(ex,data,count, FORMAT_ASCTIME);

			if (c == ',') {
				if (length == 3)
					return update(ex,data, count, "%w, %e?%b?%_ %H:%M[:%S %Z]");
				return update(ex,data, count, "%W, %e?%b?%_ %H:%M[:%S %Z]");
			}

			++length;
			READ;
			continue;
		}
		
		// length >= 10
		
		if (length == 10) {
			if (!digit)
				break;
			if (c == ' ')
				return update(ex, data, count, FORMAT_SORTABLE);
			if (!tPos) {
				digit = false;
				break;
			}
		}
	
		if (c == '.' || c == ',') {
			if (tPos==8)
				return update(ex, data, count, "%Y%m%dT%H%M%s[%z]");
			return update(ex, data, count,"%Y-%m-%dT%H:%M:%s[%z]"); //  FORMAT_ISO8601_FRAC
		}

		READ;
		++length;
	}

	if (digit) {
		if (length == 10)
			return update(ex, data, count, FORMAT_SORTABLE);
		if (tPos==10)
			return update(ex, data, count, "%Y-%m-%dT%H:%M:%S[%z]"); //  FORMAT_ISO8601
		if (tPos==8) // compact format (20050108T123000, 20050108T123000Z, 20050108T123000.123+0200)
			return update(ex, data, count, "%Y%m%dT%H%M%s[%z]");
	}
	ex.set<Ex::Format>("Impossible to determine automatically format of date ", String::Data(data, count));
	return false;
}


} // namespace Base
