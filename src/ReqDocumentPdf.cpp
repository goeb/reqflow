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

#include <fstream>
#include <poppler/cpp/poppler-document.h>
#include <poppler/cpp/poppler-page.h>
#include <poppler/cpp/poppler-global.h>
typedef std::vector<char> byte_array;


#include "ReqDocumentPdf.h"
#include "logging.h"
#include "req.h"

int ReqDocumentPdf::loadRequirements(bool debug)
{
    LOG_DEBUG("ReqDocumentPdf::loadRequirements: %s", fileConfig->path.c_str());
    init();

    poppler::document *doc = poppler::document::load_from_file(fileConfig->realpath.c_str());
	if (!doc) {
        PUSH_ERROR(fileConfig->id, "", "Cannot open file: %s", fileConfig->realpath.c_str());
        return -1;
	}
	const int pagesNbr = doc->pages();
	LOG_DEBUG("loadPdf: page count: %d", pagesNbr);

	for (int i = 0; i < pagesNbr; ++i) {
		LOG_DEBUG("page %d", i+1);
		// process the lines
		std::string pageLines;

        switch(fileConfig->encoding) {
		case LATIN1:
			pageLines = doc->create_page(i)->text().to_latin1();
			break;
		case UTF8:
		default:
		{
			byte_array pageText = doc->create_page(i)->text().to_utf8();
			pageLines.assign(pageText.begin(), pageText.end());
			break;
		}
		}

        // process one line at a time
		const char *startOfLine = pageLines.c_str();
		const char *endOfLine = 0;
		while (startOfLine) {
			endOfLine = strstr(startOfLine, "\n");
			int length = 0;
			if (endOfLine) length = endOfLine - startOfLine;
			else length = strlen(startOfLine);

			std::string line;
			line.assign(startOfLine, length);

			if (debug) {
				dumpText(line.c_str());

			} else {
				BlockStatus status = processBlock(line);
				if (status == STOP_REACHED) {
					delete doc;
					return 0;
				}
			}

			if (!endOfLine) break;
			if (endOfLine == startOfLine + strlen(startOfLine) - 1) break;
			startOfLine = endOfLine + 1;
		}
	}
    finalizeCurrentReq();

	delete doc;
    return 0;
}
