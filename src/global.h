
#ifndef _global_h
#define _global_h


#define VERSION "1.1-rc1"

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
