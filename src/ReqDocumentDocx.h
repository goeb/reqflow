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
    int loadDocxXmlNode(xmlDocPtr doc, xmlNode *a_node, bool debug);


private:
    std::string contents;
};

#endif
