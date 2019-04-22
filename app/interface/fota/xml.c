#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include <stdlib.h>
#include <ctype.h>

#include "com_app_def.h"
#include "dir.h"
#include "ftp_api.h"
#include "init.h"
#include "timer.h"
#include "shell_api.h"
#include "list.h"
#include "xml.h"

#define XML_MAX_NODES       256

#define XML_NODE_END        0
#define XML_NODE_NEW        1

#define XML_PICE_TAGS       1
#define XML_PICE_TAGE       2
#define XML_PICE_TAGV       3
#define XML_PICE_NULL       4
#define XML_PICE_ERROR      5

#define XML_BUFF_SIZE(xml)  (XML_MAX_SIZE - (xml->spos - xml->buff) - 1)


#define ARRAY_SZ(array)     (sizeof(array) / sizeof(array[0]))
#define ARRAY_LMT(array)    ((array) + ARRAY_SZ(array))

#if 0
static xml_node_t xml_nodes[XML_MAX_NODES];
static list_t xml_used;
static list_t xml_free;


static void xml_node_clean(void)
{
    xml_node_t *node;

    list_init(&xml_used);
    list_init(&xml_free);
    for (node = xml_nodes; node < ARRAY_LMT(xml_nodes); node++)
    {
        list_add_tail(&xml_free, &node->mlink);
    }
}
#endif
#if 0
static int xml_node_count(void)
{
    list_t *node;
    int count = 0;

    list_for_each(node, &xml_free, next)
    {
        count++;
    }

    return count;
}
#endif
static void xml_node_init(xml_node_t *node)
{
    node->name   = NULL;
    node->value  = NULL;
    node->parent = NULL;
    list_init(&node->xlink);
    list_init(&node->child);
}

static xml_node_t* xml_node_new(void)
{
#if 0    
    list_t *node;
    xml_node_t *ret = NULL;

    if ((node = list_get_first(&xml_free)) != NULL)
    {
        ret = list_entry(node, xml_node_t, mlink);
        list_add_tail(&xml_used, &ret->mlink);
    }

    return ret;
#else
    return (xml_node_t*)calloc(1, sizeof(xml_node_t));
#endif
}

static void xml_node_free(xml_node_t *node)
{
#if 0
    list_delete(&node->mlink);
    list_add_tail(&xml_free, &node->mlink);
#else
    free(node);
#endif
}

/*
static void xml_node_set_parent(xml_node_t *node, xml_node_t *parent, xml_t *xml)
{
    if ((node->parent = parent) == NULL)
    {
        list_add_tail(&xml->tree, &node->xlink);
    }
    else
    {
        list_add_tail(&parent->child, &node->xlink);
    }
}
*/
static void xml_node_show(xml_node_t *node, int level)
{
    int i;
    
    for (i = 0; i < level; i++)
    {
        shellprintf("  ");
    }

    shellprintf(" name  : %s\r\n", node->name ? node->name : "NULL");
    if (node->value)
    {
        for (i = 0; i < level; i++)
        {
            shellprintf("  ");
        }
        shellprintf(" value : %s\r\n", node->value);
    }
    if (!list_empty(&node->child))
    {
        list_t *cld;

        list_for_each(cld, &node->child, next)
        {
            xml_node_t *xnode = list_entry(cld, xml_node_t, xlink);
            xml_node_show(xnode, level + 1);
        }
    }
}

static char* xml_skip_space(char *str)
{
    while (isspace(*str))
    {
        str++;
    }

    return str;
}

static char* xml_back_space(char *str)
{
    while (isspace(*(str - 1)))
    {
        str--;
    }

    return str;
}


static int xml_get_pice(char **str, char *buf, int size)
{
    int ret;
    char *src = xml_skip_space(*str), *pos;

    switch (*src)
    {
        case '<':
            src = xml_skip_space(++src);
            ret = XML_PICE_TAGS;
            if (*src == '/')
            {
                src++;
                ret = XML_PICE_TAGE;
            }

            pos = src;

            while (*pos != '>')
            {
                if (isspace(*pos) || 
                    *pos == '<' || *pos == '/' || *pos == 0 || 
                    pos - src >= size)
                {
                    ret = XML_PICE_ERROR;
                    break;
                }

                pos++;
            }

            if (ret != XML_PICE_ERROR)
            {
                strncpy(buf, src, pos - src);
                buf[pos - src] = 0;
                pos++;
            }
            src = pos;
            break;
        //case '/':
        case '>':
            ret = XML_PICE_ERROR;
            break;
        case 0:
            ret = XML_PICE_NULL;
            break;
        default:
            ret = XML_PICE_TAGV;
            pos = xml_skip_space(src);

            while (*pos != 0 && *pos != '<')
            {
                /*
                if (*pos == '/' || *pos == '>')
                {
                    ret = XML_PICE_ERROR;
                    break;
                }
                */
                pos++;
            }

            if (ret != XML_PICE_ERROR)
            {
                pos = xml_back_space(pos);
                if (pos - src < size)
                {
                    strncpy(buf, src, pos - src);
                    buf[pos - src] = 0;
                }
                else
                {
                    ret = XML_PICE_ERROR;
                }
            }
            
            src = pos;
            break;
    }

    if (ret != XML_PICE_ERROR)
    {
        *str = src;
    }

    return ret;
}


static int xml_parse_line(char *line, xml_t *xml)
{
    int pice;
    char *lstr = line = xml_skip_space(line);

    if (*lstr == '#')
    {
        return 0;
    }

    while ((pice = xml_get_pice(&lstr, xml->spos, XML_BUFF_SIZE(xml))) != XML_PICE_NULL)
    {
        xml_node_t *node;
        
        switch (pice)
        {
            case XML_PICE_TAGS:
                if ((node = xml_node_new()) == NULL)
                {
                    log_e(LOG_FOTA, "no memery for new node");
                    return -1;
                }

                if (xml->curr && strcmp(xml->curr->name, xml->spos) == 0)
                {
                    log_e(LOG_FOTA, "syntax error: %s", line);
                    return -1;
                }

                xml_node_init(node);
                if ((node->parent = xml->curr) == NULL)
                {
                    xml->root = node;
                }
                else
                {
                    list_add_tail(&xml->curr->child, &node->xlink);
                }
                
                node->name = xml->spos;

                //log_o(LOG_FOTA, "tag start: %s", xml->spos);
                xml->spos += strlen(xml->spos) + 1;
                xml->curr  = node;
                //xml->stat  = XML_NODE_NEW;                
                break;
                
            case XML_PICE_TAGE:            
                if (xml->curr == NULL)
                {
                    log_e(LOG_FOTA, "syntax error: %s", line);
                    return -1;
                }
                
                if (strcmp(xml->curr->name, xml->spos) != 0)
                {
                    log_e(LOG_FOTA, "node name unmatch: %s, %s", xml->curr->name, xml->spos);
                    return -1;
                }
                
                //log_o(LOG_FOTA, "tag end: %s", xml->spos);                
                xml->spos += strlen(xml->spos) + 1;
                xml->curr  = xml->curr->parent;
                break;
                
            case XML_PICE_TAGV:
                if (xml->curr == NULL)
                {
                    log_e(LOG_FOTA, "syntax error: %s", line);
                    return -1;
                }

                if (xml->curr->value)
                {
                    log_e(LOG_FOTA, "node value redefine: %s, %s", xml->curr->value, xml->spos);
                    return -1;
                }
                
                //log_o(LOG_FOTA, "tag value: %s", xml->spos);
                
                xml->curr->value = xml->spos;
                xml->spos += strlen(xml->spos) + 1;
                break;
            case XML_PICE_ERROR:
                log_e(LOG_FOTA, "syntax error: %s", line);
                return -1;
        }
    }

    //log_o(LOG_FOTA, "xml-curr: %s", xml->curr ? xml->curr->name : "NULL");
    //log_o(LOG_FOTA, "xml-pare: %s", xml->curr && xml->curr->parent ? xml->curr->parent->name : "NULL");
    //log_o(LOG_FOTA, "xml-stat: %d", xml->stat);
    return 0;
}

xml_t* xml_load(const char *fpath)
{
    FILE *fxml;
    xml_t *xml;

    if ((fxml = fopen(fpath, "r")) == NULL)
    {
        log_e(LOG_FOTA, "can't open file: %s", fpath);
        return NULL;
    }

    if ((xml = (xml_t*)calloc(1, sizeof(xml_t))) == NULL)
    {
        log_e(LOG_FOTA, "no memory for XML loading");
    }
    else
    {        
        char lbuf[1024];
        int  lnum = 0;
        
        xml->spos = xml->buff;
        xml->curr = xml->root = NULL;

        while (fgets(lbuf, 1024, fxml))
        {
            lnum++;
            if (xml_parse_line(lbuf, xml) != 0)
            {
                log_e(LOG_FOTA, "parse line %d fail", lnum);
                xml_free(xml);
                xml = NULL;
                break;
            }
        }

        if (xml && xml->curr)
        {
            log_e(LOG_FOTA, "unexpected end of file");
            xml_free(xml);
            xml = NULL;
        }
    }

    fclose(fxml);
    return xml;
}

void xml_free(xml_t *xml)
{
    xml_node_t *node = xml->root;

    while (node)
    {
        if (!list_empty(&node->child))
        {
            node = list_entry(node->child.next, xml_node_t, xlink);
        }
        else
        {
            xml_node_t *parent = node->parent;
            list_delete(&node->xlink);
            xml_node_free(node);
            node = parent;
        }
    }

    free(xml);    
}

xml_node_t* xml_get_sub_node(xml_node_t *node, const char *name)
{
    list_t *cld;
    const char *pos = name;

    if (node == NULL || name == NULL)
    {
        return NULL;
    }

    while (*pos == '/')
    {
        pos++;
    }

    while (*pos)
    {
        list_for_each(cld, &node->child, next)
        {
            xml_node_t *sub = list_entry(cld, xml_node_t, xlink);
            int nlen = strlen(sub->name);

            if (strncmp(pos, sub->name, nlen) == 0)
            {
                if (pos[nlen] == 0)
                {
                    return sub;
                }
                else if (pos[nlen] == '/')
                {
                    pos += nlen;
                    node = sub;
                    break;
                }
            }
        }

        if (*pos != '/')
        {
            break;
        }
        
        pos++;
    }

    return NULL;
}

char* xml_get_sub_value(xml_node_t *node, const char *name)
{
    xml_node_t *sub = xml_get_sub_node(node, name);    
    return sub ? sub->value : NULL;
}

int xml_get_sub_node_lst(xml_node_t *node, const char *name, xml_node_t **lst, int max)
{
    list_t *cld;
    int cnt = 0;

    list_for_each(cld, &node->child, next)
    {
        xml_node_t *sub = list_entry(cld, xml_node_t, xlink);
        
        if (max <= 0)
        {
            break;
        }

        if (strcmp(sub->name, name) == 0)
        {
            max--;
            cnt++;
            *lst++ = sub;
        }
    }

    return cnt;
}

int xml_get_sub_node_cnt(xml_node_t *node, const char *name)
{
    list_t *cld;
    int cnt = 0;

    list_for_each(cld, &node->child, next)
    {
        xml_node_t *sub = list_entry(cld, xml_node_t, xlink);
        

        if (strcmp(sub->name, name) == 0)
        {
            cnt++;
        }
    }

    return cnt;
}

xml_node_t* xml_get_node(xml_t *xml, const char *name)
{
    /*
    list_t *cld, *tree = &xml->tree;
    const char *pos = name;

    while (*pos == '/')
    {
        pos++;
    }

    while (*pos)
    {
        list_for_each(cld, tree, next)
        {
            xml_node_t *sub = list_entry(cld, xml_node_t, xlink);
            int nlen = strlen(sub->name);

            if (strncmp(pos, sub->name, nlen) == 0)
            {
                if (pos[nlen] == 0)
                {
                    return sub;
                }
                else if (pos[nlen] == '/')
                {
                    pos += nlen;
                    tree = &sub->child;
                    break;
                }
            }
        }

        if (*pos != '/')
        {
            break;
        }
        
        pos++;
    }

    return NULL;
    */

    int nlen;
    
    if (xml == NULL || name == NULL || xml->root == NULL)
    {
        return NULL;
    }

    while (*name == '/')
    {
        name++;
    }

    nlen = strlen(xml->root->name);
    
    if (strncmp(xml->root->name, name, nlen) == 0)
    {
        if (name[nlen] == 0)
        {
            return xml->root;
        }

        if (name[nlen] == '/')
        {
            return xml_get_sub_node(xml->root, name + nlen);
        }
    }
    
    return NULL;
}

char* xml_get_value(xml_t *xml, const char *name)
{
    xml_node_t *sub = xml_get_node(xml, name);    
    return sub ? sub->value : NULL;
}


void xml_show(xml_t *xml)
{
    if (xml->root)
    {
        xml_node_show(xml->root, 0);
    }
}

static int xml_test(int argc, const char **argv)
{
    xml_t *xml;
    
    if (argc != 1)
    {
        shellprintf(" usage: xmltst <file path>\r\n");
        return -1;
    }
    
    if ((xml = xml_load(argv[0])) == NULL)
    {
        shellprintf(" error: load XML file fail");
        return -1;
    }
    
    xml_show(xml);
    xml_free(xml);
    return 0;
}

int xml_init(int phase)
{
    int ret = 0;

    switch (phase)
    {
        case INIT_PHASE_INSIDE:
            //xml_node_clean();
            break;

        case INIT_PHASE_OUTSIDE:
            ret |= shell_cmd_register("xmltst", xml_test, "test XML loader");
        default:
            break;
    }

    return ret;
}
