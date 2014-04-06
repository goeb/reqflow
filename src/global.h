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

#ifndef _global_h
#define _global_h


#define VERSION "1.3.0"

// prepare for gettext
#define _(String) (String)

#define FOREACH(var, container) for (var=container.begin(); var!= container.end(); var++)


#ifdef _WIN32
inline struct tm *localtime_r(const time_t *timep, struct tm *result)
{
    struct tm *lt = localtime(timep);
    *result = *lt;
    return result;
}
#endif



#endif
