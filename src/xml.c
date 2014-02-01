
#include <stdio.h>
#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

/**
 * print_element_names:
 * @a_node: the initial xml node to consider.
 *
 * Prints the names of the all the xml elements
 * that are siblings or children of a given xml node.
 */
static void print_element_names(xmlDocPtr doc, xmlNode * a_node)
{
    xmlNode *cur_node = NULL;

    for (cur_node = a_node; cur_node; cur_node = cur_node->next) {
        if (cur_node->type == XML_ELEMENT_NODE) {
            printf("node type: Element, name: %s\n", cur_node->name);
        } else if (XML_TEXT_NODE == cur_node->type) {
            xmlChar *key;
            //key = xmlNodeListGetString(doc, cur_node, 1);
            key = xmlNodeListGetRawString(doc, cur_node, 1);
            printf("text node: %s\n", key);
            xmlFree(key);
        }

        print_element_names(doc, cur_node->children);
    }
}
int main(int argc, char **argv)
{
    LIBXML_TEST_VERSION
    char           *filename;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s filename.xml\n", argv[0]);
        return 1;
    }
    filename = argv[1];

    xmlDocPtr         document;
    xmlNode        *root, *first_child, *node;

    document = xmlReadFile(filename, NULL, 0);
    root = xmlDocGetRootElement(document);

    print_element_names(document, root);

    fprintf(stdout, "Root is <%s> (%i)\n", root->name, root->type);
    first_child = root->children;
    for (node = first_child; node; node = node->next) {
        fprintf(stdout, "\t Child is <%s> (%i)\n", node->name, node->type);
    }
    fprintf(stdout, "...\n");
    return 0;
}
