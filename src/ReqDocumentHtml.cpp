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

#include <string.h>
#include "ReqDocumentHtml.h"
#include "logging.h"
#include "parseConfig.h"
#include "stringTools.h"

/**
  * @param[in/out] text
  */
BlockStatus ReqDocumentHtml::processParagraph(std::string &text, bool inParagaph, bool debug)
{
    LOG_DEBUG("processParagraph: inParagaph=%d", inParagaph);
    LOG_DEBUG("processParagraph: text=%s", text.c_str());

    BlockStatus s = REQ_OK;

    if (debug) {
         dumpText(text.c_str());
    } else {
        // process text of paragraph
        if (inParagaph) s = processBlock(text);
        else {
            // process line per line
            LOG_DEBUG("processParagraph line per line, size=%zd", text.size());
            size_t pos0 = 0;
            while (pos0 < text.size()) {
                std::string line;
                size_t pos1 = text.find('\n', pos0);
                if (pos1 == std::string::npos) {
                    // take last line
                    line = text.substr(pos0);
                    pos0 = text.size(); // stop the loop
                } else {
                    line = text.substr(pos0, pos1-pos0);
                    pos0 = pos1 + 1;
                }

                s = processBlock(line);
                if (s == STOP_REACHED) break;
            }
        }
    }

    text.clear();
    return s;
}

const char *HtmlParagraphBoundaries[] = {
    // some HTML block level elements
    "address",
    "article",
    "aside",
    "blockquote",
    "dd",
    "div",
    "dl",
    "fieldset",
    "figcaption",
    "figure",
    "footer",
    "form",
    "h1",
    "h2",
    "h3",
    "h4",
    "h5",
    "h6",
    "header",
    "hgroup",
    "hr",
    "li",
    "main",
    "ol",
    "output" ,
    "p",
    "section",
    "table",
    "tfoot",
    "ul",
    // other elements
    "td",
    NULL // the list must end with NULL
};

/** Tell if a HTML node name is considered as a paragraph boundary
 */
static bool isParagraphBoundary(const std::string &nodeName)
{
    const char **ptr = HtmlParagraphBoundaries;
    while (*ptr) {
        if (nodeName == *ptr) return true;
        ptr++;
    }
    return false;
}

/** Parsing is done as follows:
  * - if inside a <p>, process paragraph as a whole
  * - else, process line per line
  */
BlockStatus ReqDocumentHtml::loadHtmlNode(xmlDocPtr doc, xmlNode *a_node, bool inParagraph, bool debug)
{
    LOG_FUNC();
    xmlNode *currentNode = NULL;
    BlockStatus s;

    static std::string currentText; // text of current paragraph, consolidated over recursive calls

    LOG_DEBUG("loadHtmlNode: inParagraph=%d", inParagraph);
    for (currentNode = a_node; currentNode; currentNode = currentNode->next) {
        std::string nodeName;
        if (currentNode->name) nodeName = (char*)currentNode->name;

        if (nodeName == "body") {
            if (!fileConfig->startAfterRegex) acquisitionStarted = true;

        } else if (nodeName == "br") {
            // stay in the same paragraph
            // add a newline
            currentText += "\n";

        } else if (isParagraphBoundary(nodeName)) {
            if (!currentText.empty()) {
                // process previous text (that was not in a paragraph)
                s = processParagraph(currentText, false, debug);
                if (s == STOP_REACHED) return STOP_REACHED;
            }
            inParagraph = true;
        }

        if (currentNode->type == XML_ELEMENT_NODE) {
            // recurse through children
            LOG_DEBUG("node: %s", (char*)currentNode->name);
            s = loadHtmlNode(doc, currentNode->children, inParagraph, debug);
            if (s == STOP_REACHED) return STOP_REACHED;

            if (!currentText.empty() && isParagraphBoundary(nodeName)) {
                // process this paragraph
                s = processParagraph(currentText, true, debug);
                if (s == STOP_REACHED) return STOP_REACHED;

            } else if (!currentText.empty() && nodeName == "body") {
                // process the text that was in no paragraph
                s = processParagraph(currentText, false, debug);
                if (s == STOP_REACHED) return STOP_REACHED;
                finalizeCurrentReq();
            }

        } else if (XML_TEXT_NODE == currentNode->type) {
            // accumulate text in current paragraph
            xmlChar *text;
            text = xmlNodeGetContent(currentNode);
            LOG_DEBUG("text size: %zd bytes", strlen((char*)text));
            //LOG_DEBUG("text: %s", (char*)text);

            currentText += (char*)text;

            //LOG_DEBUG("textInParagraphCurrent: [%s]", currentText.c_str());

            xmlFree(text);
        }
    }
    return REQ_OK;
}


int ReqDocumentHtml::loadRequirements(bool debug)
{
    xmlDocPtr pDoc = 0;
    xmlNodePtr pRoot = 0;

    LOG_DEBUG("htmlParseFile(%s)...", fileConfig->realpath.c_str());
    pDoc = htmlParseFile(fileConfig->realpath.c_str(), (const char*)"utf-8"); // TODO support other than UTF-8

    if (!pDoc) {
        PUSH_ERROR(fileConfig->id, "", "Cannot open/parse HTML file: %s", fileConfig->realpath.c_str());
        return -1;
    }
    pRoot = xmlDocGetRootElement(pDoc);

    if (!pRoot) {
        PUSH_ERROR(fileConfig->id, "", "Cannot get Root of HTML file: %s", fileConfig->realpath.c_str());
        return -1;
    }

    return loadHtmlNode(pDoc, pRoot, false, debug);
}

void ReqDocumentHtml::init()
{
    acquisitionStarted = false; // should become true in <body>
    currentRequirement = "";
}

