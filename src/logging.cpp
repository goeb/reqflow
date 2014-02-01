
#include <string.h>

#include "logging.h"

bool doPrint(enum LogLevel msgLevel)
{
    const char *envLevel = getenv("SMIT_DEBUG");
    enum LogLevel policy = LL_INFO; // default value
    if (envLevel) {
        if (0 == strcmp(envLevel, "FATAL")) policy = LL_FATAL;
        else if (0 == strcmp(envLevel, "ERROR")) policy = LL_ERROR;
        else if (0 == strcmp(envLevel, "INFO")) policy = LL_INFO;
        else if (0 == strcmp(envLevel, "DEBUG")) policy = LL_DEBUG;
        else policy = LL_INFO;
    }
    return (policy >= msgLevel);
}

