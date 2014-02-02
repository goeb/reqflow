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

std::map<std::string, ReqFileConfig> ReqConfig;

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



int main(int argc, const char **argv)
{
    if (argc < 2) usage();

    const char *command = argv[1];

    if (0 == strcmp(command, "stat"))         return cmdStat(argc-2, argv+2);
    else if (0 == strcmp(command, "version")) return showVersion();
    else if (0 == strcmp(command, "list"))    return cmdList(argc-2, argv+2);
    else if (0 == strcmp(command, "config"))    return cmdConfig(argc-2, argv+2);
    else usage();

    return 0;
}
