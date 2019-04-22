#ifndef __XML_H__
#define __XML_H__
#include "list.h"

#define XML_MAX_SIZE        65536

typedef struct xml_node
{
    char *name;
    char *value;
    list_t mlink;
    list_t xlink;
    list_t child;
    struct xml_node *parent;
} xml_node_t;


typedef struct
{
    char buff[XML_MAX_SIZE];
    char *spos;
    int stat;
    xml_node_t *curr;
    xml_node_t *root;
} xml_t;

extern int xml_init(int phase);
extern xml_t* xml_load(const char *fpath);
extern void xml_free(xml_t *xml);
extern xml_node_t* xml_get_sub_node(xml_node_t *node, const char *name);
extern xml_node_t* xml_get_node(xml_t *xml, const char *name);
extern char* xml_get_sub_value(xml_node_t *node, const char *name);
extern char* xml_get_value(xml_t *xml, const char *name);
extern int xml_get_sub_node_cnt(xml_node_t *node, const char *name);
extern int xml_get_sub_node_lst(xml_node_t *node, const char *name, xml_node_t **lst, int max);
#endif
