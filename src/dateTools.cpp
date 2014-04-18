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
#include "config.h"

#include <sys/time.h>
#include <time.h>
#include <stdio.h>

#include "global.h"
#include "dateTools.h"

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

std::string getDatetime()
{
    struct tm date;
    struct timeval tv;
    gettimeofday(&tv, 0);
    localtime_r(&tv.tv_sec, &date);
    //int milliseconds = tv.tv_usec / 1000;
    char buffer[100];
    sprintf(buffer, "%.4d-%.2d-%.2d %.2d:%.2d:%.2d", date.tm_year + 1900,
            date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
    return buffer;
}

/** Return timestamp suitable for a file path
  * Format: 20141231-235959
  */
std::string getDatetimePath()
{
    struct tm date;
    struct timeval tv;
    gettimeofday(&tv, 0);
    localtime_r(&tv.tv_sec, &date);
    //int milliseconds = tv.tv_usec / 1000;
    char buffer[100];
    sprintf(buffer, "%.4d%.2d%.2d-%.2d%.2d%.2d", date.tm_year + 1900,
            date.tm_mon + 1, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
    return buffer;
}
