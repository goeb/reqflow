
#include <zip.h>
#include <string.h>

#include "ReqDocumentDocx.h"
#include "logging.h"
#include "parseConfig.h"

int ReqDocumentDocxXml::loadDocxXmlNode(xmlDocPtr doc, xmlNode *a_node, bool debug)
{
    xmlNode *currentNode = NULL;

    static std::string textInParagraphCurrent; // consolidated over recursive calls

    for (currentNode = a_node; currentNode; currentNode = currentNode->next) {
        std::string nodeName;
        if (currentNode->type == XML_ELEMENT_NODE) {
            if (0 == strcmp((char*)currentNode->name, "pStyle")) {
                xmlAttr* attribute = currentNode->properties;
                while(attribute && attribute->name && attribute->children)
                {
                    xmlChar* style = xmlNodeListGetString(currentNode->doc, attribute->children, 1);
                    LOG_DEBUG("style: %s", (char*)style);


                    xmlFree(style);
                    attribute = attribute->next;
                }

            } // take benefit of styles in the future
            LOG_DEBUG("node: %s", (char*)currentNode->name);
            nodeName = (char*)currentNode->name;

        } else if (XML_TEXT_NODE == currentNode->type) {
            xmlChar *text;
            text = xmlNodeListGetRawString(doc, currentNode, 1);
            LOG_DEBUG("text size: %d bytes", strlen((char*)text));
            LOG_DEBUG("text: %s", (char*)text);

            textInParagraphCurrent += (char*)text;

            LOG_DEBUG("textInParagraphCurrent: %s", textInParagraphCurrent.c_str());

            xmlFree(text);
        }

        loadDocxXmlNode(doc, currentNode->children, debug);

        if (nodeName =="p" && !textInParagraphCurrent.empty()) {
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


int ReqDocumentDocxXml::loadRequirements(bool debug)
{
    const char *xml;
    int r = loadFile(fileConfig.path.c_str(), &xml);
    if (r <= 0) {
        LOG_ERROR("Cannot read file (or empty): %s", fileConfig.path.c_str());
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

    LOG_DEBUG("loadDocx: %s", fileConfig.path.c_str());
    int err;

    struct zip *zipFile = zip_open(fileConfig.path.c_str(), 0, &err);
    if (!zipFile) {
        LOG_ERROR("Cannot open file: %s", fileConfig.path.c_str());
        return -1;
    }

    const char *CONTENTS = "word/document.xml";
    int i = zip_name_locate(zipFile, CONTENTS, 0);
    if (i < 0) {
        LOG_DEBUG("Not a Open XML document (missing word/document.xml). Trying Open Document.");
		const char *OPEN_DOC_CONTENTS = "content.xml";
		i = zip_name_locate(zipFile, OPEN_DOC_CONTENTS, 0);
		if (i < 0) {
			LOG_ERROR("Not a valid docx document; %s", fileConfig.path.c_str());
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
        LOG_DEBUG("%s:%s: %d bytes", fileConfig.path.c_str(), CONTENTS, contents.size());

        zip_fclose(fileInZip);
    } else {
        LOG_ERROR("Cannot open file %d in zip: %s", i, fileConfig.path.c_str());
    }
    zip_close(zipFile);

    // parse the XML
    ReqDocumentDocxXml docXml(fileConfig);
    return docXml.loadContents(contents.data(),contents.size(), debug);
}
