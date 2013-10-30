#include "htable.h"

#include <stdlib.h>

htable *htable_new(int buckets, hash h, match m, destroy d)
{
    if (buckets <= 0)
        return NULL;

    struct htable *ht = (struct htable *)malloc(sizeof(htable));
    if (NULL == ht) {
        return NULL;
    }

    ht->tbl = (struct htable_item **)malloc(buckets * sizeof(struct htable_item*));
    if (NULL == ht->tbl) {
        free(ht);
        return NULL;
    }

    ht->buckets = buckets;

    for (int i = 0; i < buckets; i++) {
        ht->tbl[i] = NULL;
    }

    ht->h = h;
    ht->m = m;
    ht->d = d;

    ht->sz = 0;

    return ht;
}

void htable_free(htable *ht)
{
    for (int i = 0; i < ht->buckets; i++) {
        struct htable_item *item = ht->tbl[i];
        struct htable_item *prev;
        while (item) {
            prev = item;
            item = item->next;
            ht->d(prev->key, prev->data);
            free(prev);
        }
    }

    free(ht->tbl);
}

int htable_insert(htable *ht, void *key, void *data)
{
    void *tmp;

    if (0 == htable_find(ht, key, &tmp))
        return -1;

    int bucket = ht->h(key) % ht->buckets;
    struct htable_item *item = (struct htable_item *)malloc(sizeof(struct htable_item));
    if (NULL == item)
        return -1;

    item->next = ht->tbl[bucket];
    ht->tbl[bucket] = item;
    ht->sz++;
    return 0;
}

int htable_remove(htable *ht, void *key)
{
    int bucket = ht->h(key) % ht->buckets;
    struct htable_item *item = ht->tbl[bucket];
    struct htable_item *prev = NULL;
    while (item) {
        if (0 == ht->m(key, item->key))
        {
            if (prev)
                prev->next = item->next;
            ht->d(item->key, item->data);
            return 0;
        }
        prev = item;
        item = item->next;
    }
    return -1;
}

int htable_find(htable *ht, void *key, void **data)
{
    int bucket = ht->h(key) % ht->buckets;
    struct htable_item *item = ht->tbl[bucket];
    while (item) {
        if (0 == ht->m(key, item->key))
            break;
        item = item->next;
    }
    *data = item;
    return item ? 0 : -1;
}

unsigned int pjwhash(void *key)
{
    char *ptr;
    unsigned int val;

    val = 0;
    ptr = (char *)key;

    while (*ptr != '\0')
    {
        unsigned int tmp;
        val = (val << 4) + (*ptr);

        if (tmp = (val & 0xf0000000))
        {
            val = val ^ (tmp >> 24);
            val = val ^ tmp;
        }
        ptr++;
    }

    return val % PRIME_TABLE_SIZE;
}

int match1(void *k1, void *k2)
{
    if (k1 == k2)
        return 0;
    return -1;
}

void destroy1(void *key, void *data)
{

}
