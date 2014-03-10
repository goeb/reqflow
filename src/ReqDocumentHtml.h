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

#ifndef _importerHtml_h
#define _importerHtml_h

#include <string>
#include <libxml/HTMLparser.h>
#include <libxml/tree.h>

#include "req.h"


class ReqDocumentHtml: public ReqDocument {
public:
    ReqDocumentHtml(ReqFileConfig &c) {fileConfig = &c;}
    int loadRequirements(bool debug);
    BlockStatus processParagraph(std::string &text, bool inParagaph, bool debug);
    BlockStatus loadHtmlNode(xmlDocPtr doc, xmlNode *a_node, bool inParagraph, bool debug);
    void init();
};

#endif
