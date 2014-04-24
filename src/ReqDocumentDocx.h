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

#ifndef _importerDocx_h
#define _importerDocx_h

#include <string>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "req.h"


class ReqDocumentDocx: public ReqDocument {
public:
    ReqDocumentDocx(ReqFileConfig &c) {fileConfig = &c;}
    int loadRequirements(bool debug);
};

class ReqDocumentDocxXml: public ReqDocument {
public:
    ReqDocumentDocxXml(ReqFileConfig &c) {fileConfig = &c;}
    int loadRequirements(bool debug);
    int loadContents(const char *xml, size_t size, bool debug);
    int loadDocxXmlNode(xmlDocPtr doc, xmlNode *node, bool debug);


private:
    std::string contents;
};

#endif
