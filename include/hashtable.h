/****************************************************************
file:         hashtable.h
description:  the header file of hashtable definition
date:         2016/10/26
author        yuzhimin
****************************************************************/
#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

typedef struct entry
{
    unsigned int h;     // hash value
    unsigned int k;     // key
    unsigned int v;     // value
    unsigned int used;
    struct entry *next;
} HASHTABLE_ENTRY, *PHASHTABLE_ENTRY;

typedef struct hashtable
{
    unsigned int loadlimit;
    unsigned int tablelength;
    unsigned int entrycount;
    struct entry *table;
} HASHTABLE, *PHASHTABLE;

/* indexFor */
static inline unsigned int indexFor(unsigned int tablelength, unsigned int hashvalue)
{
    return (hashvalue % tablelength);
};

/*****************************************************************************
 * create_hashtable

 * @name                    hashtable_init
 * @param   size            initial size of hashtable
 * @return                  newly created hashtable or NULL on failure
 */

int hashtable_init(HASHTABLE *h, unsigned int limit, HASHTABLE_ENTRY *e, unsigned int size);

/*****************************************************************************
 * hashtable_insert

 * @name        hashtable_insert
 * @param   h   the hashtable to insert into
 * @param   k   the key - hashtable claims ownership and will free on removal
 * @param   v   the value - does not claim ownership
 * @return      non-zero for successful insertion
 *
 * This function will cause the table to expand if the insertion would take
 * the ratio of entries to table size over the maximum load factor.
 *
 * This function does not check for repeated insertions with a duplicate key.
 * The value returned when using a duplicate key is undefined -- when
 * the hashtable changes size, the order of retrieval of duplicate key
 * entries is reversed.
 * If in doubt, remove before insert.
 */

int hashtable_insert(struct hashtable *h, unsigned int k, unsigned int v);

/*****************************************************************************
 * hashtable_search

 * @name        hashtable_search
 * @param   h   the hashtable to search
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or NULL if none found
 */

int hashtable_search(struct hashtable *h, unsigned int k, unsigned int *v);

/*****************************************************************************
 * hashtable_remove

 * @name        hashtable_remove
 * @param   h   the hashtable to remove the item from
 * @param   k   the key to search for  - does not claim ownership
 * @return      the value associated with the key, or NULL if none found
 */

int hashtable_remove(struct hashtable *h, unsigned int k, unsigned int *v);

/*****************************************************************************
 * hashtable_count

 * @name        hashtable_count
 * @param   h   the hashtable
 * @return      the number of items stored in the hashtable
 */
unsigned int hashtable_count(struct hashtable *h);

#endif

