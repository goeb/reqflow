/*   Req
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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <regex.h>
#include <map>
#include <set>
#include <fstream>


#include "global.h"
#include "logging.h"
#include "parseConfig.h"

void usage()
{
    printf("Usage: req <command> [<options>] [<args>]\n"
           "\n"
           "Commands:\n"
           "\n"
           "    stat\n"
           "\n"
           "    list\n"
           "\n"
           "    config\n"
           "\n"
           "    regex <pattern> <text>\n"
           "          Test regex given by <pattern> applied on <text>\n"
           "\n"
           "    version\n"
           "    help\n"
           "\n"
           "Options:\n"
           "    -c <config> Select configuration file.\n"
           "\n"
           "\n");
    exit(1);
}

int showVersion()
{
    printf("Small Issue Tracker v%s\n"
           "Copyright (C) 2013 Frederic Hoerni\n"
           "\n"
           "This program is free software; you can redistribute it and/or modify\n"
           "it under the terms of the GNU General Public License as published by\n"
           "the Free Software Foundation; either version 2 of the License, or\n"
           "(at your option) any later version.\n"
           "\n"
           "This program is distributed in the hope that it will be useful,\n"
           "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
           "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
           "GNU General Public License for more details.\n"
           , VERSION);
    exit(1);
}

struct ReqFileConfig {
    std::string id;
    std::string path;
    std::string tagPattern;
    regex_t tagRegex;
    std::string refPattern;
    regex_t refRegex;
    std::list<std::string> dependencies;
};

struct Requirement {
    std::string id;
    std::string parentDocumentId;
    std::set<std::string> coveredRequirements;
};

std::map<std::string, ReqFileConfig> ReqConfig;
std::map<std::string, Requirement> Requirements;

std::string pop(std::list<std::string> & L)
{
    std::string token = "";
    if (!L.empty()) {
        token = L.front();
        L.pop_front();
    }
    return token;
}

int loadConfiguration(const char * file)
{
    LOG_DEBUG(_("Loading configuration file: '%s'"), file);

    const char *config;
    int r = loadFile(file, &config);
    if (r<0) {
        LOG_ERROR(_("Cannot load file '%s': %s"), file, strerror(errno));
        return 1;
    } else if (r == 0) {
        LOG_ERROR(_("Empty configuration file '%s'."), file);
        return 1;
    }
    // parse the configuration
    std::list<std::list<std::string> > configTokens = parseConfigTokens(config, r);

    std::list<std::list<std::string> >::iterator line;
    int lineNum = 0;
    FOREACH(line, configTokens) {
        lineNum++;
        std::string verb = pop(*line);
        if (verb == "addFile") {
            ReqFileConfig fileConfig;
            fileConfig.id = pop(*line);
            LOG_DEBUG("addFile '%s'...", fileConfig.id.c_str());
            if (fileConfig.id.empty()) {
                LOG_ERROR("Missing identifier for file: line %d", lineNum);
            }

            while (!line->empty()) {
                std::string arg = pop(*line);
                if (arg == "-path") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -path value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.path = pop(*line);
                } else if (arg == "-tag") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -tag value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.tagPattern = pop(*line);
                    /* Compile regular expression */
                    int reti = regcomp(&fileConfig.tagRegex, fileConfig.tagPattern.c_str(), 0);
                    if (reti) {
                        LOG_ERROR("Cannot compile tag regex for %s: %s", fileConfig.id.c_str(), fileConfig.tagPattern.c_str());
                        exit(1);
                    }


                } else if (arg == "-ref") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -ref value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.refPattern = pop(*line);
                    /* Compile regular expression */
                    int reti = regcomp(&fileConfig.refRegex, fileConfig.refPattern.c_str(), 0);
                    if (reti) {
                        LOG_ERROR("Cannot compile ref regex for %s: %s", fileConfig.id.c_str(), fileConfig.refPattern.c_str());
                        exit(1);
                    }


                } else if (arg == "-depends-on") {
                    if (line->empty()) {
                        LOG_ERROR("Missing -depends-on value for %s", fileConfig.id.c_str());
                        return -1;
                    }
                    fileConfig.dependencies.insert(fileConfig.dependencies.begin(), line->begin(), line->end());
                    line->clear();
                } else {
                    LOG_ERROR("Invalid token '%s': line %d", arg.c_str(), lineNum);
                }
            }
            std::map<std::string, ReqFileConfig>::iterator c = ReqConfig.find(fileConfig.id);
            if (c != ReqConfig.end()) {
                LOG_ERROR("Config error: duplicate id '%s'", fileConfig.id.c_str());
                return 1;
            }
            ReqConfig[fileConfig.id] = fileConfig;

        } else {
            LOG_ERROR("Invalid token '%s': line %d", verb.c_str(), lineNum);
        }
    }
    return 0;
}

enum ReqFileType { RF_TEXT, RF_ODT, RF_DOCX, RF_XSLX, RF_UNKNOWN };
ReqFileType getFileType(const std::string &path)
{
    size_t i = path.find_last_of('.');
    if (i == std::string::npos) return RF_UNKNOWN;
    if (i == path.size()-1) return RF_UNKNOWN;
    std::string extension = path.substr(i+1);
    if (0 == strcasecmp(extension.c_str(), "txt")) return RF_TEXT;
    else if (0 == strcasecmp(extension.c_str(), "odt")) return RF_ODT;
    else if (0 == strcasecmp(extension.c_str(), "docx")) return RF_DOCX;
    else if (0 == strcasecmp(extension.c_str(), "xslx")) return RF_XSLX;
    else return RF_UNKNOWN;
}

std::string getMatchingPattern(regex_t *regex, const char *text)
{
    std::string matchingText;
    // check if line contains a requirement
    const int N = 5; // max number of groups
    regmatch_t pmatch[N];
    int reti = regexec(regex, text, N, pmatch, 0);
    if (!reti) {
        // take the last group, becauase we want to support the 2 following cases.
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
        const int LINE_MAX = 4096;
        char buffer[LINE_MAX];
        for (i=0; i<N; i++) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                if (length > LINE_MAX-1) {
                    LOG_ERROR("Requirement size too big (%d)", length);
                    break;
                }
                memcpy(buffer, text+pmatch[i].rm_so, length);
                buffer[length] = 0;
                LOG_DEBUG("match[%d]: %s", i, buffer);
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


void loadText(ReqFileConfig &fileConfig)
{
    const int LINE_MAX = 4096;
    char line[LINE_MAX];

    std::ifstream ifs(fileConfig.path.c_str(), std::ifstream::in);

    std::string currentRequirement;


    if (!ifs.good()) {
        LOG_ERROR("Cannot open file: %s", fileConfig.path.c_str());
        return;
    }
    while (ifs.getline(line, LINE_MAX)) {

        std::string requirementId = getMatchingPattern(&fileConfig.tagRegex, line);

        if (!requirementId.empty()) {

            std::map<std::string, Requirement>::iterator r = Requirements.find(requirementId);
            if (r != Requirements.end()) {
                LOG_ERROR("Duplicate requirement %s in documents: '%s' and '%s'",
                          requirementId.c_str(), r->second.parentDocumentId.c_str(), fileConfig.path.c_str());
                currentRequirement.clear();

            } else {
                Requirement req;
                req.id = requirementId;
                req.parentDocumentId = fileConfig.path;
                Requirements[requirementId] = req;
                currentRequirement = requirementId;
            }
        }

        // check if line covers a requirement
        std::string ref = getMatchingPattern(&fileConfig.refRegex, line);
        if (!ref.empty()) {
            if (currentRequirement.empty()) {
                LOG_ERROR("Reference found whereas no current requirement.");
            } else {
                Requirements[currentRequirement].coveredRequirements.insert(ref);
            }
        }
    }

}
void loadDocx(ReqFileConfig &fileConfig)
{

}

int loadRequirements()
{
    std::map<std::string, ReqFileConfig>::iterator c;
    FOREACH(c, ReqConfig) {
        ReqFileConfig &fileConfig = c->second;
        ReqFileType fileType = getFileType(fileConfig.path);
        switch(fileType) {
        case RF_TEXT:
            loadText(fileConfig);
            break;
        case RF_DOCX:
            loadDocx(fileConfig);
            break;
        default:
            LOG_ERROR("Cannot load unsupported file type: %s", fileConfig.path.c_str());
            return -1;
        }
    }
    return 0;
}

int cmdStat(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = 0;
    const char *arg = 0;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        }
    }

    int r = loadConfiguration(configFile);
    if (r != 0) return 1;

    return 0;
}

int cmdList(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = 0;
    const char *arg = 0;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        }
    }
    if (!configFile) {
        LOG_ERROR(_("No configuration file specified. Please provide a -c option."));
        return 1;
    } else {
        int r = loadConfiguration(configFile);
        if (r != 0) return 1;
    }

    int r = loadRequirements();
    if (r != 0) return 1;


    return 0;
}

int cmdConfig(int argc, const char **argv)
{
    int i = 0;
    const char *configFile = 0;
    const char *arg = 0;
    while (i<argc) {
        arg = argv[i]; i++;
        if (0 == strcmp(arg, "-c")) {
            if (i>=argc) usage();
            configFile = argv[i]; i++;
        }
    }
    if (!configFile) {
        LOG_ERROR(_("No configuration file specified. Please provide a -c option."));
        return 1;
    } else {
        int r = loadConfiguration(configFile);
        if (r != 0) return 1;
    }

    // display a summary of the configuration
    std::map<std::string, ReqFileConfig>::iterator c;
    FOREACH(c, ReqConfig) {
        printf("%s: %s\n", c->first.c_str(), c->second.path.c_str());
    }

    return 0;
}

int cmdRegex(int argc, const char **argv)
{
    if (argc != 2) usage();

    regex_t regex;
    int reti;

    /* Compile regular expression */
    reti = regcomp(&regex, argv[0], 0);
    if (reti) {
        LOG_ERROR("Could not compile regex");
        return 1;
    }

    /* Execute regular expression */

    const int N = 5;

    char buffer[256];
    const char *text = argv[1];
    regmatch_t pmatch[N];
    reti = regexec(&regex, text, N, pmatch, 0);
    if (!reti) {
        printf("Match: \n");
        int i;
        for (i=0; i<N; i++) {
            if (pmatch[i].rm_so != -1) {
                int length = pmatch[i].rm_eo - pmatch[i].rm_so;
                memcpy(buffer, text+pmatch[i].rm_so, length);
                buffer[length] = 0;
                printf("match[%d]: %s\n", i, buffer);
            } else printf("match[%d]:\n", i);
        }
    } else if (reti == REG_NOMATCH) {
        printf("No match");

    } else {
        char msgbuf[100];
        regerror(reti, &regex, msgbuf, sizeof(msgbuf));
        LOG_ERROR("Regex match failed: %s", msgbuf);
        return 1;
    }

    /* Free compiled regular expression if you want to use the regex_t again */
    regfree(&regex);

    return 0;
}



int main(int argc, const char **argv)
{
    if (argc < 2) usage();

    const char *command = argv[1];

    if (0 == strcmp(command, "stat"))         return cmdStat(argc-2, argv+2);
    else if (0 == strcmp(command, "version")) return showVersion();
    else if (0 == strcmp(command, "list"))    return cmdList(argc-2, argv+2);
    else if (0 == strcmp(command, "config"))  return cmdConfig(argc-2, argv+2);
    else if (0 == strcmp(command, "regex"))   return cmdRegex(argc-2, argv+2);
    else usage();


    return 0;
}
