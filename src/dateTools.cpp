/*   Small Issue Tracker
 *   Copyright (C) 2013 Frederic Hoerni
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
#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#include "global.h"
#include "dateTools.h"

std::string getLocalTimestamp()
{

    struct tm t;
    time_t now = time(0);
    localtime_r(&now, &t);

    const int SIZ = 30;
    char buffer[SIZ+1];
    size_t n = strftime(buffer, SIZ, "%Y-%m-%d %H:%M:%S", &t);

    if (n != 19) return "invalid-timestamp";

    // add milliseconds
    struct timeval tv;
    gettimeofday(&tv, 0);
    int milliseconds = tv.tv_usec/1000;

    snprintf(buffer+n, SIZ-n, ".%03d", milliseconds);

    return buffer;
}

std::string epochToString(time_t t)
{
    struct tm *tmp;
    tmp = localtime(&t);
    char datetime[100+1]; // should be enough
    strftime(datetime, sizeof(datetime)-1, "%d %b %Y, %H:%M:%S", tmp);

    return std::string(datetime);
}

/** Return the duration since that time
  * @return "x days", or "x months", or "x hours", etc.
  */
std::string epochToStringDelta(time_t t)
{
    char datetime[100+1]; // should be enough
    //strftime(datetime, sizeof(datetime)-1, "%Y-%m-%d %H:%M:%S", tmp);
    time_t delta = time(0) - t;
    if (delta < 60) snprintf(datetime, sizeof(datetime)-1, "%ld %s", delta, _("s"));
    else if (delta < 60*60) snprintf(datetime, sizeof(datetime)-1, "%ld %s", delta/60, _("min"));
    else if (delta < 60*60*24) snprintf(datetime, sizeof(datetime)-1, "%ld %s", delta/60/60, _("h"));
    else if (delta < 60*60*24*31) snprintf(datetime, sizeof(datetime)-1, "%ld %s", delta/60/60/24, _("day"));
    else if (delta < 60*60*24*365) snprintf(datetime, sizeof(datetime)-1, "%ld %s", delta/60/60/24/30, _("month"));
    else if (delta > 0) snprintf(datetime, sizeof(datetime)-1, "%ld %s", delta/60/60/24/30/365, _("year"));
    else return epochToString(t);


    return std::string(datetime);
}
