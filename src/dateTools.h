#ifndef _dateTools_h
#define _dateTools_h

#include <string>

std::string epochToString(time_t t);
std::string epochToStringDelta(time_t t);
std::string getDatetime();

#endif
