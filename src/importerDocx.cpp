
#include <zip.h>
#include <string.h>

#include "importerDocx.h"
#include "logging.h"

void loadDocxXmlNode(ReqFileConfig &fileConfig, xmlDocPtr doc, xmlNode *a_node, std::map<std::string, Requirement> &requirements)
{
    xmlNode *currentNode = NULL;

    static std::string currentRequirement;
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



        loadDocxXmlNode(fileConfig, doc, currentNode->children, requirements);

        if (nodeName =="p" && !textInParagraphCurrent.empty()) {
            // process text of paragraph

            // check coverage of requirement
            std::string ref = getMatchingPattern(fileConfig.refRegex, textInParagraphCurrent.c_str());
            if (!ref.empty()) {
                if (currentRequirement.empty()) {
                    LOG_ERROR("Reference found whereas no current requirement: %s", ref.c_str());
                } else {
                    requirements[currentRequirement].covers.insert(ref);
                }
            }

            // check plain requirement
            std::string reqId = getMatchingPattern(fileConfig.tagRegex, textInParagraphCurrent.c_str());

            // TODO if reqId and ref are the same (same value && same offset), only consider ref.
            if (!reqId.empty() && (reqId != ref) ) {

                std::map<std::string, Requirement>::iterator r = requirements.find(reqId);
                if (r != requirements.end()) {
                    LOG_ERROR("Duplicate requirement %s in documents: '%s' and '%s'",
                              reqId.c_str(), r->second.parentDocumentPath.c_str(), fileConfig.path.c_str());
                    currentRequirement.clear();

                } else {
                    Requirement req;
                    req.id = reqId;
                    req.parentDocumentId = fileConfig.id;
                    req.parentDocumentPath = fileConfig.path;
                    requirements[reqId] = req;
                    currentRequirement = reqId;
                }
            }

            textInParagraphCurrent.clear();
        }
    }
}


void loadDocxXml(ReqFileConfig &fileConfig, const std::string &contents, std::map<std::string, Requirement> &requirements)
{
    xmlDocPtr document;
    xmlNode *root;

    document = xmlReadMemory(contents.data(), contents.size(), 0, 0, 0);
    root = xmlDocGetRootElement(document);

    loadDocxXmlNode(fileConfig, document, root, requirements);
}



void loadDocx(ReqFileConfig &fileConfig, std::map<std::string, Requirement> &requirements)
{
    LOG_DEBUG("loadDocx: %s", fileConfig.path.c_str());
    int err;

    struct zip *zipFile = zip_open(fileConfig.path.c_str(), 0, &err);
    if (!zipFile) {
        LOG_ERROR("Cannot open file: %s", fileConfig.path.c_str());
        return;
    }

    const char *CONTENTS = "word/document.xml";
    int i = zip_name_locate(zipFile, CONTENTS, 0);
    if (i < 0) {
        LOG_ERROR("Not a valid docx document; %s", fileConfig.path.c_str());
        zip_close(zipFile);
        return;
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
    loadDocxXml(fileConfig, contents, requirements);
}
