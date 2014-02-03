
#include "req.h"
#include "logging.h"

// static objects
std::map<std::string, ReqFileConfig> ReqConfig;
std::map<std::string, Requirement> Requirements;


std::string getMatchingPattern(regex_t *regex, const char *text)
{
    if (!regex) return "";

    std::string matchingText;
    // check if line matches
    const int N = 5; // max number of groups
    regmatch_t pmatch[N];
    //LOG_DEBUG("regexec(%p, %p)", regex, text);
    int reti = regexec(regex, text, N, pmatch, 0);
    if (!reti) {
        // take the last group, because we want to support the 2 following cases.
        // Example 1: ./req regex "<\(REQ_[-a-zA-Z_0-9]*\)>" "ex: <REQ_123> (comment)"
        // match[0]: <REQ_123>
        // match[1]: REQ_123
        // match[2]:
        //
        // Example 2: ./req regex "REQ_[-a-zA-Z_0-9]*" "ex: REQ_123 (comment)"
        // match[0]: REQ_123
        // match[1]:
        // match[2]:

        int i;
        const int LINE_SIZE_MAX = 4096;
        char buffer[LINE_SIZE_MAX];
        for (i=0; i<N; i++) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                if (length > LINE_SIZE_MAX-1) {
                    LOG_ERROR("Requirement size too big (%d)", length);
                    break;
                }
                memcpy(buffer, text+pmatch[i].rm_so, length);
                buffer[length] = 0;
                //LOG_DEBUG("match[%d]: %s", i, buffer);
                matchingText = buffer; // overwrite each time, so that we take the last one

            } else break; // no more groups
        }

    } else if (reti == REG_NOMATCH) {
        matchingText = "";

    } else {
        char msgbuf[1024];
        regerror(reti, regex, msgbuf, sizeof(msgbuf));
        LOG_ERROR("Regex match failed: %s", msgbuf);
        matchingText = "";
    }

    return matchingText;
}
