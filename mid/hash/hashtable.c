/****************************************************************
file:         hashtable.c
description:  the source file of hashtable implementation
date:         2016/10/26
author        yuzhimin
****************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hashtable.h"

static const unsigned int primes[] =
{
    53, 97, 193, 389,
    769, 1543, 3079, 6151,
    12289, 24593, 49157, 98317,
    196613, 393241, 786433, 1572869,
    3145739, 6291469, 12582917, 25165843,
    50331653, 100663319, 201326611, 402653189,
    805306457, 1610612741
};

/*****************************************************************************/
int hashtable_init(HASHTABLE *h, unsigned int limit, HASHTABLE_ENTRY *e, unsigned int size)
{
    /* Check requested hashtable isn't too large */
    if (size > (1u << 30))
    {
        return -1;
    }

    if (NULL == h || NULL == e)
    {
        return -1;
    }

    h->table = e;
    memset(h->table, 0, size * sizeof(HASHTABLE_ENTRY));
    h->loadlimit    = limit;
    h->tablelength  = size;
    h->entrycount   = 0;
    return 0;
}

/*****************************************************************************/
unsigned int hash(struct hashtable *h, unsigned int k)
{
    /* Aim to protect against poor hash functions by adding logic here
     * - logic taken from java 1.4 hashtable source */
    unsigned int i = k;
    i += ~(i << 9);
    i ^= ((i >> 14) | (i << 18));  /* >>> */
    i += (i << 4);
    i ^= ((i >> 10) | (i << 22));  /* >>> */
    return i;
}

/*****************************************************************************/
unsigned int hashtable_count(struct hashtable *h)
{
    return h->entrycount;
}

/*****************************************************************************/
int hashtable_insert(struct hashtable *h, unsigned int k, unsigned int v)
{
    /* This method allows duplicate keys - but they shouldn't be used */
    unsigned int hashvalue, index, i;
    struct entry *e;

    if (++(h->entrycount) > h->loadlimit)
    {
        h->entrycount--;
        return -1;
    }

    hashvalue = hash(h, k);
    index = indexFor(h->loadlimit, hashvalue);
    e = &(h->table[index]);

    if (!(e->used))
    {
        e->h = hashvalue;
        e->k = k;
        e->v = v;
        e->used = 1;
    }
    else
    {
        while (NULL != e->next)
        {
            if (k == e->k)
            {
                return -2;
            }

            e = e->next;
        }

        // find not used element
        for (i = h->loadlimit; i < h->tablelength; i++)
        {
            if (!(h->table[i].used))
            {
                break;
            }
        }

        if (i >= h->tablelength)
        {
            return -3;
        }

        e->next = &(h->table[i]);
        e = &(h->table[i]);
        e->h = hashvalue;
        e->k = k;
        e->v = v;
        e->used = 1;
    }

    return 0;
}

/*****************************************************************************/
/* returns value associated with key */
int hashtable_search(struct hashtable *h, unsigned int k, unsigned int *v)
{
    struct entry *e;
    unsigned int hashvalue, index;
    hashvalue = hash(h, k);
    index = indexFor(h->loadlimit, hashvalue);
    e = &(h->table[index]);

    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (k == e->k))
        {
            *v = e->v;
            return 0;
        };

        e = e->next;
    }

    return -1;
}

/*****************************************************************************/
/* return remove success or failed */
int hashtable_remove(struct hashtable *h, unsigned int k, unsigned int *v)
{
    /* TODO: consider compacting the table when the load factor drops enough,
     *       or provide a 'compact' method. */

    struct entry *e;
    struct entry *pE;
    unsigned int hashvalue, index;

    hashvalue = hash(h, k);
    index = indexFor(h->loadlimit, hash(h, k));
    pE = &(h->table[index]);
    e = pE;

    while (NULL != e)
    {
        /* Check hash value to short circuit heavier comparison */
        if ((hashvalue == e->h) && (k == e->k))
        {
            pE = e->next;
            h->entrycount--;
            *v = e->v;
            e->used = 0;
            return 0;
        }

        pE = e->next;
        e = e->next;
    }

    return -1;
}

