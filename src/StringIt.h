/*   Reqflow
 *   Copyright (C) 2014 Frederic Hoerni
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 2 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

/** String Iterator
  *
  */

#ifndef _stringIt_h
#define _stringIt_h

#include <string>



class StringIt {
public:
    enum Options {
        COLLAPSE_SEP, // multiple consecutive separators are to be considered as one
        ANY_SEP = 2, // each of the characters in the 'separator' is a separator
                     // otherwise the separator is a multi-character separator
    };
    StringIt();
    StringIt(const std::string& str, const char *separator, int opts = 0);
    std::string next(const char *alternateSeparator = 0);
private:
    const std::string &currentString;
    size_t currentPosition;
    int options;
    const char *separator;
};



#endif
