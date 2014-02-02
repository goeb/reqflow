
#ifndef _logging_h
#define _logging_h

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#include "dateTools.h"

enum LogLevel {
    LL_FATAL,
    LL_ERROR,
    LL_INFO,
    LL_DEBUG
};

bool doPrint(enum LogLevel msgLevel);

#define LOG_ERROR(...) do { if (doPrint(LL_ERROR)) { LOG("ERROR", __VA_ARGS__); } } while (0)
#define LOG_INFO(...)  do { if (doPrint(LL_INFO)) { LOG("INFO ", __VA_ARGS__); } } while (0)
#define LOG_DEBUG(...) do { if (doPrint(LL_DEBUG)) { LOG("DEBUG", __VA_ARGS__); } } while (0)

#define LOG(_level, ...) if (doPrint(LL_DEBUG)) LOG_FULL(_level, __FILE__, __LINE__, __VA_ARGS__); \
    else { LOG_SHORT(_level, __VA_ARGS__); }

#define LOG_FULL(_level, _file, _line, ...) do { \
    fprintf(stderr, "%s %s %s:%d ", getLocalTimestamp().c_str(), _level, _file, _line); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    } while (0)

#define LOG_SHORT(_level, ...) do { \
    fprintf(stderr, "%s ", _level); \
    fprintf(stderr, __VA_ARGS__); \
    fprintf(stderr, "\n"); \
    } while (0)



class FuncScope {
public:
    FuncScope(const char *file, int line, const char *name) {
        funcName = name;  if (doPrint(LL_DEBUG)) { LOG_FULL("FUNC ", file, line, "Entering %s...", funcName); }
    }
    ~FuncScope() { if (doPrint(LL_DEBUG)) { LOG("FUNC ", "Leaving %s...", funcName); } }
private:
    const char *funcName;
};

#define LOG_FUNC() FuncScope __fs(__FILE__, __LINE__, __func__);


#endif
