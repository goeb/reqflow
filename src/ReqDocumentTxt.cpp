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

#include <fstream>

#include "ReqDocumentTxt.h"
#include "logging.h"
#include "req.h"

int ReqDocumentTxt::loadRequirements(bool debug)
{
    init();

    LOG_DEBUG("loadText: %s", fileConfig->path.c_str());
    const int LINE_SIZE_MAX = 4096;
    char line[LINE_SIZE_MAX];

    std::ifstream ifs(fileConfig->path.c_str(), std::ifstream::in);

    currentRequirement = "";

    if (!ifs.good()) {
        LOG_ERROR("Cannot open file: %s", fileConfig->path.c_str());
        return -1;
    }

	int linenum = 1;
    while (ifs.getline(line, LINE_SIZE_MAX)) { // stop if line too long
		if (debug) {
			printf("%s", line);

		} else {
            std::string L = line;
            BlockStatus status = processBlock(L);
			if (status == STOP_REACHED) return 0;
		}
		linenum++;
    }
	if (!ifs.eof()) {
        LOG_ERROR("Line too long in file '%s': %d (max size=%d)", fileConfig->path.c_str(), linenum, LINE_SIZE_MAX);
	}
    finalizeCurrentReq();

    return 0;
}
