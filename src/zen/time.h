// *****************************************************************************
// * This file is part of the FreeFileSync project. It is distributed under    *
// * GNU General Public License: https://www.gnu.org/licenses/gpl-3.0          *
// * Copyright (C) Zenju (zenju AT freefilesync DOT org) - All Rights Reserved *
// *****************************************************************************

#ifndef TIME_H_8457092814324342453627
#define TIME_H_8457092814324342453627

#include <ctime>
#include "string_tools.h"


namespace zen
{
struct TimeComp //replaces std::tm and SYSTEMTIME
{
    int year   = 0; // -
    int month  = 0; //1-12
    int day    = 0; //1-31
    int hour   = 0; //0-23
    int minute = 0; //0-59
    int second = 0; //0-60 (including leap second)
};

inline bool operator==(const TimeComp& lhs, const TimeComp& rhs)
{
    return lhs.year == rhs.year && lhs.month == rhs.month  && lhs.day == rhs.day && lhs.hour == rhs.hour && lhs.minute == rhs.minute && lhs.second == rhs.second;
}

TimeComp getLocalTime(time_t utc = std::time(nullptr)); //convert time_t (UTC) to local time components
time_t   localToTimeT(const TimeComp& comp);            //convert local time components to time_t (UTC), returns -1 on error

TimeComp getUtcTime(time_t utc = std::time(nullptr)); //convert time_t (UTC) to UTC time components
time_t   utcToTimeT(const TimeComp& comp);            //convert UTC time components to time_t (UTC), returns -1 on error

TimeComp getCompileTime();

//----------------------------------------------------------------------------------------------------------------------------------

/*
format (current) date and time; example:
        formatTime<std::wstring>(L"%Y|%m|%d"); -> "2011|10|29"
        formatTime<std::wstring>(FORMAT_DATE); -> "2011-10-29"
        formatTime<std::wstring>(FORMAT_TIME); -> "17:55:34"
*/
template <class String, class String2>
String formatTime(const String2& format, const TimeComp& comp = getLocalTime()); //format as specified by "std::strftime", returns empty string on failure

//the "format" parameter of formatTime() is partially specialized with the following type tags:
const struct FormatDateTag     {} FORMAT_DATE      = {}; //%x - locale dependent date representation: e.g. 08/23/01
const struct FormatTimeTag     {} FORMAT_TIME      = {}; //%X - locale dependent time representation: e.g. 14:55:02
const struct FormatDateTimeTag {} FORMAT_DATE_TIME = {}; //%c - locale dependent date and time:       e.g. Thu Aug 23 14:55:02 2001

const struct FormatIsoDateTag     {} FORMAT_ISO_DATE      = {}; //%Y-%m-%d          - e.g. 2001-08-23
const struct FormatIsoTimeTag     {} FORMAT_ISO_TIME      = {}; //%H:%M:%S          - e.g. 14:55:02
const struct FormatIsoDateTimeTag {} FORMAT_ISO_DATE_TIME = {}; //%Y-%m-%d %H:%M:%S - e.g. 2001-08-23 14:55:02

//----------------------------------------------------------------------------------------------------------------------------------

template <class String, class String2>
bool parseTime(const String& format, const String2& str, TimeComp& comp); //similar to ::strptime(), return true on success

//----------------------------------------------------------------------------------------------------------------------------------
















//############################ implementation ##############################
namespace implementation
{
inline
std::tm toClibTimeComponents(const TimeComp& comp)
{
    assert(1 <= comp.month  && comp.month  <= 12 &&
           1 <= comp.day    && comp.day    <= 31 &&
           0 <= comp.hour   && comp.hour   <= 23 &&
           0 <= comp.minute && comp.minute <= 59 &&
           0 <= comp.second && comp.second <= 61);

    std::tm ctc = {};
    ctc.tm_year  = comp.year - 1900; //years since 1900
    ctc.tm_mon   = comp.month - 1;   //0-11
    ctc.tm_mday  = comp.day;         //1-31
    ctc.tm_hour  = comp.hour;        //0-23
    ctc.tm_min   = comp.minute;      //0-59
    ctc.tm_sec   = comp.second;      //0-60 (including leap second)
    ctc.tm_isdst = -1;               //> 0 if DST is active, == 0 if DST is not active, < 0 if the information is not available
    //ctc.tm_wday
    //ctc.tm_yday
    return ctc;
}

inline
TimeComp toZenTimeComponents(const std::tm& ctc)
{
    TimeComp comp;
    comp.year   = ctc.tm_year + 1900;
    comp.month  = ctc.tm_mon + 1;
    comp.day    = ctc.tm_mday;
    comp.hour   = ctc.tm_hour;
    comp.minute = ctc.tm_min;
    comp.second = ctc.tm_sec;
    return comp;
}


template <class T>
struct GetFormat; //get default time formats as char* or wchar_t*

template <>
struct GetFormat<FormatDateTag> //%x - locale dependent date representation: e.g. 08/23/01
{
    const char*    format(char)    const { return  "%x"; }
    const wchar_t* format(wchar_t) const { return L"%x"; }
};

template <>
struct GetFormat<FormatTimeTag> //%X - locale dependent time representation: e.g. 14:55:02
{
    const char*    format(char)    const { return  "%X"; }
    const wchar_t* format(wchar_t) const { return L"%X"; }
};

template <>
struct GetFormat<FormatDateTimeTag> //%c - locale dependent date and time:       e.g. Thu Aug 23 14:55:02 2001
{
    const char*    format(char)    const { return  "%c"; }
    const wchar_t* format(wchar_t) const { return L"%c"; }
};

template <>
struct GetFormat<FormatIsoDateTag> //%Y-%m-%d - e.g. 2001-08-23
{
    const char*    format(char)    const { return  "%Y-%m-%d"; }
    const wchar_t* format(wchar_t) const { return L"%Y-%m-%d"; }
};

template <>
struct GetFormat<FormatIsoTimeTag> //%H:%M:%S - e.g. 14:55:02
{
    const char*    format(char)    const { return  "%H:%M:%S"; }
    const wchar_t* format(wchar_t) const { return L"%H:%M:%S"; }
};

template <>
struct GetFormat<FormatIsoDateTimeTag> //%Y-%m-%d %H:%M:%S - e.g. 2001-08-23 14:55:02
{
    const char*    format(char)    const { return  "%Y-%m-%d %H:%M:%S"; }
    const wchar_t* format(wchar_t) const { return L"%Y-%m-%d %H:%M:%S"; }
};


//strftime() craziness on invalid input:
//  VS 2010: CRASH unless "_invalid_parameter_handler" is set: https://msdn.microsoft.com/en-us/library/ksazx244.aspx
//  GCC: returns 0, apparently no crash. Still, considering some clib maintainer's comments, we should expect the worst!
inline
size_t strftimeWrap_impl(char* buffer, size_t bufferSize, const char* format, const std::tm* timeptr)
{
    return std::strftime(buffer, bufferSize, format, timeptr);
}


inline
size_t strftimeWrap_impl(wchar_t* buffer, size_t bufferSize, const wchar_t* format, const std::tm* timeptr)
{
    return std::wcsftime(buffer, bufferSize, format, timeptr);
}

/*
inline
bool isValid(const std::tm& t)
{
     -> not enough! MSCRT has different limits than the C standard which even seem to change with different versions:
        _VALIDATE_RETURN((( timeptr->tm_sec >=0 ) && ( timeptr->tm_sec <= 59 ) ), EINVAL, FALSE)
        _VALIDATE_RETURN(( timeptr->tm_year >= -1900 ) && ( timeptr->tm_year <= 8099 ), EINVAL, FALSE)
    -> also std::mktime does *not* help here at all!

    auto inRange = [](int value, int minVal, int maxVal) { return minVal <= value && value <= maxVal; };

    //http://www.cplusplus.com/reference/clibrary/ctime/tm/
    return inRange(t.tm_sec,  0, 61) &&
           inRange(t.tm_min,  0, 59) &&
           inRange(t.tm_hour, 0, 23) &&
           inRange(t.tm_mday, 1, 31) &&
           inRange(t.tm_mon,  0, 11) &&
           //tm_year
           inRange(t.tm_wday, 0, 6) &&
           inRange(t.tm_yday, 0, 365);
    //tm_isdst
};
*/

template <class CharType> inline
size_t strftimeWrap(CharType* buffer, size_t bufferSize, const CharType* format, const std::tm* timeptr)
{

    return strftimeWrap_impl(buffer, bufferSize, format, timeptr);
}


struct UserDefinedFormatTag {};
struct PredefinedFormatTag  {};

template <class String, class String2> inline
String formatTime(const String2& format, const TimeComp& comp, UserDefinedFormatTag) //format as specified by "std::strftime", returns empty string on failure
{
    using CharType = typename GetCharType<String>::Type;
    std::tm ctc = toClibTimeComponents(comp);
    std::mktime(&ctc); // unfortunately std::strftime() needs all elements of "struct tm" filled, e.g. tm_wday, tm_yday
    //note: although std::mktime() explicitly expects "local time", calculating weekday and day of year *should* be time-zone and DST independent

    CharType buffer[256] = {};
    const size_t charsWritten = strftimeWrap(buffer, 256, strBegin(format), &ctc);
    return String(buffer, charsWritten);
}

template <class String, class FormatType> inline
String formatTime(FormatType, const TimeComp& comp, PredefinedFormatTag)
{
    using CharType = typename GetCharType<String>::Type;
    return formatTime<String>(GetFormat<FormatType>().format(CharType()), comp, UserDefinedFormatTag());
}
}


inline
TimeComp getLocalTime(time_t utc)
{
    std::tm comp = {};
    if (::localtime_r(&utc, &comp) == nullptr)
        return TimeComp();

    return implementation::toZenTimeComponents(comp);
}


inline
TimeComp getUtcTime(time_t utc)
{
    std::tm comp = {};
    if (::gmtime_r(&utc, &comp) == nullptr)
        return TimeComp();

    return implementation::toZenTimeComponents(comp);
}


inline
time_t localToTimeT(const TimeComp& comp) //returns -1 on error
{
    std::tm ctc = implementation::toClibTimeComponents(comp);
    return std::mktime(&ctc);
}


inline
time_t utcToTimeT(const TimeComp& comp) //returns -1 on error
{
    std::tm ctc = implementation::toClibTimeComponents(comp);
    ctc.tm_isdst = 0; //"Zero (0) to indicate that standard time is in effect" => unused by _mkgmtime, but take no chances
    return ::timegm(&ctc);
}


inline
TimeComp getCompileTime()
{
    //https://gcc.gnu.org/onlinedocs/cpp/Standard-Predefined-Macros.html
    char compileTime[] = __DATE__ " " __TIME__; //e.g. "Aug  1 2017 01:32:26"
    if (compileTime[4] == ' ') //day is space-padded, but %d expects zero-padding
        compileTime[4] = '0';

    TimeComp tc = {};
    if (parseTime("%b %d %Y %H:%M:%S", compileTime, tc))
        return tc;

    assert(false);
    return TimeComp();
}


template <class String, class String2> inline
String formatTime(const String2& format, const TimeComp& comp)
{
    using FormatTag = typename SelectIf<
                      IsSameType<String2, FormatDateTag       >::value ||
                      IsSameType<String2, FormatTimeTag       >::value ||
                      IsSameType<String2, FormatDateTimeTag   >::value ||
                      IsSameType<String2, FormatIsoDateTag    >::value ||
                      IsSameType<String2, FormatIsoTimeTag    >::value ||
                      IsSameType<String2, FormatIsoDateTimeTag>::value, implementation::PredefinedFormatTag, implementation::UserDefinedFormatTag>::Type;

    return implementation::formatTime<String>(format, comp, FormatTag());
}


template <class String, class String2>
bool parseTime(const String& format, const String2& str, TimeComp& comp) //return true on success
{
    using CharType = typename GetCharType<String>::Type;
    static_assert(IsSameType<CharType, typename GetCharType<String2>::Type>::value, "");

    const CharType*       itFmt = strBegin(format);
    const CharType* const fmtLast = itFmt + strLength(format);

    const CharType*       itStr = strBegin(str);
    const CharType* const strLast = itStr + strLength(str);

    auto extractNumber = [&](int& result, size_t digitCount) -> bool
    {
        if (strLast - itStr < makeSigned(digitCount))
            return false;

        if (std::any_of(itStr, itStr + digitCount, [](CharType c) { return !isDigit(c); }))
        return false;

        result = zen::stringTo<int>(StringRef<const CharType>(itStr, itStr + digitCount));
        itStr += digitCount;
        return true;
    };

    for (; itFmt != fmtLast; ++itFmt)
    {
        const CharType fmt = *itFmt;

        if (fmt == '%')
        {
            ++itFmt;
            if (itFmt == fmtLast)
                return false;

            switch (*itFmt)
            {
                case 'Y':
                    if (!extractNumber(comp.year, 4))
                        return false;
                    break;
                case 'm':
                    if (!extractNumber(comp.month, 2))
                        return false;
                    break;
                case 'b': //abbreviated month name: Jan-Dec
                {
                    if (strLast - itStr < 3)
                        return false;

                    const char* months[] = { "jan", "feb", "mar", "apr", "may", "jun", "jul", "aug", "sep", "oct", "nov", "dec" };
                    auto itMonth = std::find_if(std::begin(months), std::end(months), [&](const char* name)
                    {
                        return asciiToLower(itStr[0]) == name[0] &&
                               asciiToLower(itStr[1]) == name[1] &&
                               asciiToLower(itStr[2]) == name[2];
                    });
                    if (itMonth == std::end(months))
                        return false;

                    comp.month = 1 + static_cast<int>(itMonth - std::begin(months));
                    itStr += 3;
                }
                break;
                case 'd':
                    if (!extractNumber(comp.day, 2))
                        return false;
                    break;
                case 'H':
                    if (!extractNumber(comp.hour, 2))
                        return false;
                    break;
                case 'M':
                    if (!extractNumber(comp.minute, 2))
                        return false;
                    break;
                case 'S':
                    if (!extractNumber(comp.second, 2))
                        return false;
                    break;
                default:
                    return false;
            }
        }
        else if (isWhiteSpace(fmt)) //single whitespace in format => skip 0..n whitespace chars
        {
            while (itStr != strLast && isWhiteSpace(*itStr))
                ++itStr;
        }
        else
        {
            if (itStr == strLast || *itStr != fmt)
                return false;
            ++itStr;
        }
    }

    return itStr == strLast;
}
}

#endif //TIME_H_8457092814324342453627
