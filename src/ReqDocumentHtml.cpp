
#include <string.h>
#include "ReqDocumentHtml.h"
#include "logging.h"
#include "parseConfig.h"
#include "stringTools.h"

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
            LOG_DEBUG("processParagraph line per line, size=%d", text.size());
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
            // add a newline
            currentText += "\n";

        } else if (nodeName == "p") {
            // process previous text (that was not in a paragraph)
            s = processParagraph(currentText, false, debug);
            if (s == STOP_REACHED) return STOP_REACHED;
            inParagraph = true;
        }

        if (currentNode->type == XML_ELEMENT_NODE) {
            // recurse through children
            LOG_DEBUG("node: %s", (char*)currentNode->name);
            s = loadHtmlNode(doc, currentNode->children, inParagraph, debug);
            if (s == STOP_REACHED) return STOP_REACHED;

            if (nodeName == "p") {
                // process this paragraph
                s = processParagraph(currentText, true, debug);
                if (s == STOP_REACHED) return STOP_REACHED;

            } else if (nodeName == "body") {
                // process the text that was in no paragraph
                s = processParagraph(currentText, false, debug);
                if (s == STOP_REACHED) return STOP_REACHED;
            }

        } else if (XML_TEXT_NODE == currentNode->type) {
            // accumulate text in current paragraph
            xmlChar *text;
            text = xmlNodeGetContent(currentNode);
            LOG_DEBUG("text size: %d bytes", strlen((char*)text));
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

    LOG_DEBUG("htmlParseFile(%s)...", fileConfig->path.c_str());
    pDoc = htmlParseFile(fileConfig->path.c_str(), (const char*)"utf-8"); // TODO support other than UTF-8

    if (!pDoc) {
        LOG_ERROR("Cannot open/parse HTML file: %s", fileConfig->path.c_str());
        return -1;
    }
    pRoot = xmlDocGetRootElement(pDoc);

    if (!pRoot) {
        LOG_ERROR("Cannot get Root of HTML file: %s", fileConfig->path.c_str());
        return -1;
    }

    return loadHtmlNode(pDoc, pRoot, false, debug);
}

void ReqDocumentHtml::init()
{
    acquisitionStarted = false; // should become true in <body>
    currentRequirement = "";
}

