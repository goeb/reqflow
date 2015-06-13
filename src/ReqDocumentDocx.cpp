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

#include <zip.h>
#include <string.h>

#include "ReqDocumentDocx.h"
#include "logging.h"
#include "parseConfig.h"

/** Analyse a node of the document
  *
  */
int ReqDocumentDocxXml::loadDocxXmlNode(xmlDocPtr doc, xmlNode *node, bool debug)
{
    static std::string textInParagraphCurrent; // consolidated over recursive calls
    std::string nodeName;

    if (node->type == XML_ELEMENT_NODE) {
        if (0 == strcmp((char*)node->name, "pStyle")) {
            xmlAttr* attribute = node->properties;
            while(attribute && attribute->name && attribute->children)
            {
                xmlChar* style = xmlNodeListGetString(node->doc, attribute->children, 1);
                LOG_DEBUG("style: %s", (char*)style);

                xmlFree(style);
                attribute = attribute->next;
            }
            // TODO take benefit of styles in the future
        } else if (0 == strcmp((char*)node->name, "del")) return 0; // ignore deleted text (revision marks)
        else if (0 == strcmp((char*)node->name, "moveFrom")) return 0; // ignore deleted text (revision marks)

        LOG_DEBUG("node: %s", (char*)node->name);
        nodeName = (char*)node->name;

        xmlNode *subnode = NULL;
        for (subnode = node->children; subnode; subnode = subnode->next) {

            // recursively go down the xml structure
            loadDocxXmlNode(doc, subnode, debug);
        }

    } else if (XML_TEXT_NODE == node->type) {
        xmlChar *text;
        text = xmlNodeGetContent(node);
        LOG_DEBUG("text size: %zd bytes", strlen((char*)text));
        LOG_DEBUG("text: %s", (char*)text);

        textInParagraphCurrent += (char*)text;

        LOG_DEBUG("textInParagraphCurrent: %s", textInParagraphCurrent.c_str());

        xmlFree(text);
    }


    if (nodeName =="p" && !textInParagraphCurrent.empty()) {
        if (debug) {
            dumpText(textInParagraphCurrent.c_str());

        } else {
            // process text of paragraph
            BlockStatus status = processBlock(textInParagraphCurrent);
            if (status == STOP_REACHED) return 0;
        }

        textInParagraphCurrent.clear();

    } else if (nodeName =="document" || nodeName =="document-content") {
        // end of document
        finalizeCurrentReq();
    }
    return 0;
}


int ReqDocumentDocxXml::loadRequirements(bool debug)
{
    const char *xml;
    int r = loadFile(fileConfig->realpath.c_str(), &xml);
    if (r <= 0) {
        PUSH_ERROR(fileConfig->id, "", "Cannot read file (or empty): %s", fileConfig->realpath.c_str());
        return -1;
    }
    loadContents(xml, r, debug);
    free((void*)xml);
    return 0;
}

int ReqDocumentDocxXml::loadContents(const char *xml, size_t size, bool debug)
{
    init();

    xmlDocPtr document;
    xmlNode *root;

    document = xmlReadMemory(xml, size, 0, 0, 0);
    root = xmlDocGetRootElement(document);

	return loadDocxXmlNode(document, root, debug);
}

int ReqDocumentDocx::loadRequirements(bool debug)
{
    init();

    LOG_DEBUG("loadDocx: %s", fileConfig->realpath.c_str());
    int err;

    struct zip *zipFile = zip_open(fileConfig->realpath.c_str(), 0, &err);
    if (!zipFile) {
        PUSH_ERROR(fileConfig->id, "", "Cannot open file: %s", fileConfig->realpath.c_str());
        return -1;
    }

    const char *CONTENTS = "word/document.xml";
    int i = zip_name_locate(zipFile, CONTENTS, 0);
    if (i < 0) {
        // I suppose here something to be confirmed (or not):
        // MS Word 2010 stores OpenDocument files with suffix .docx.
        // So we cannot say at first sight if a docx is an OpenXml or an OpenDocument.
        // Therefore we try OpenXml first (by looking at word/document.xml)
        // and in case of failure, we try OpenDocument (by looking at content.xml)

        // From a wikipedia page I see that I am probably wrong.
        // If so, this second try should be removed.
        LOG_DEBUG("Not a Open XML document (missing word/document.xml). Trying Open Document.");
		const char *OPEN_DOC_CONTENTS = "content.xml";
		i = zip_name_locate(zipFile, OPEN_DOC_CONTENTS, 0);
		if (i < 0) {
            PUSH_ERROR(fileConfig->id, "", "Not a valid docx document: %s", fileConfig->realpath.c_str());
			zip_close(zipFile);
			return -1;
		}
    }

    std::string contents; // buffer for loading the XML contents
    struct zip_file *fileInZip = zip_fopen_index(zipFile, i, 0);
    if (fileInZip) {
        const int BUF_SIZ = 4096;
        char buffer[BUF_SIZ];
        int r;
        while ( (r = zip_fread(fileInZip, buffer, BUF_SIZ)) > 0) {
            contents.append(buffer, r);
        }
        LOG_DEBUG("%s:%s: %zd bytes", fileConfig->path.c_str(), CONTENTS, contents.size());

        zip_fclose(fileInZip);
    } else {
        PUSH_ERROR(fileConfig->id, "", "Cannot open file %d in zip: %s", i, fileConfig->realpath.c_str());
    }
    zip_close(zipFile);

    // parse the XML
    ReqDocumentDocxXml docXml(*fileConfig);
    return docXml.loadContents(contents.data(),contents.size(), debug);
}
