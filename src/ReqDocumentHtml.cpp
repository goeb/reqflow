
#include <string.h>
#include "ReqDocumentHtml.h"
#include "logging.h"
#include "parseConfig.h"
#include "stringTools.h"

int ReqDocumentHtml::loadHtmlNode(xmlDocPtr doc, xmlNode *a_node, bool debug)
{
    xmlNode *currentNode = NULL;

    static std::string textInParagraphCurrent; // consolidated over recursive calls

    for (currentNode = a_node; currentNode; currentNode = currentNode->next) {
        std::string nodeName;
        if (currentNode->type == XML_ELEMENT_NODE) {
            LOG_DEBUG("node: %s", (char*)currentNode->name);
            nodeName = (char*)currentNode->name;

        } else if (XML_TEXT_NODE == currentNode->type) {
            xmlChar *text;
            text = xmlNodeListGetRawString(doc, currentNode, 1);
            LOG_DEBUG("text size: %d bytes", strlen((char*)text));
            LOG_DEBUG("text: %s", (char*)text);

            textInParagraphCurrent += (char*)text;

            LOG_DEBUG("textInParagraphCurrent: [%s]", textInParagraphCurrent.c_str());

            xmlFree(text);
        }

        loadHtmlNode(doc, currentNode->children, debug);

        if (0 == icompare(nodeName, "p") && !textInParagraphCurrent.empty()) {
			if (debug) {
				 dumpText(textInParagraphCurrent.c_str());

			} else {
				// process text of paragraph
				BlockStatus status = processBlock(textInParagraphCurrent);
				if (status == STOP_REACHED) return 0;
			}

            textInParagraphCurrent.clear();
        }
    }
    return 0;
}


int ReqDocumentHtml::loadRequirements(bool debug)
{
    xmlDocPtr pDoc = 0;
    xmlNodePtr pRoot = 0;

    pDoc = htmlParseFile(fileConfig.path.c_str(), (const char*)"utf-8"); // TODO support other than UTF-8

    if (!pDoc) {
        LOG_ERROR("Cannot open/parse HTML file: %s", fileConfig.path.c_str());
        return -1;
    }
    pRoot = xmlDocGetRootElement(pDoc);

    if (!pRoot) {
        LOG_ERROR("Cannot get Root of HTML file: %s", fileConfig.path.c_str());
        return -1;
    }

    return loadHtmlNode(pDoc, pRoot, debug);
}


