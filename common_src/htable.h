#ifndef HTABLE_H_INCLUDED
#define HTABLE_H_INCLUDED

struct htable_item {
    void *key;
    void *data;
    struct htable_item *next;
};

typedef unsigned int (*hash)(void *);
typedef int (*match)(void *, void *);
typedef void (*destroy)(void *, void *);

struct htable {
    int buckets;
    hash h;
    match m;
    destroy d;
    int sz;
    struct htable_item **tbl;
};

htable *htable_new(int buckets, hash h, match m, destroy d);
void htable_free(htable *ht);
int htable_insert(htable *ht, void *key, void *data);
int htable_remove(htable *ht, void *key);
int htable_find(htable *ht, void *key, void **data);
int htable_size(htable *ht);

/* string hash */
#define PRIME_TABLE_SIZE 1024
unsigned int pjwhash(void *key);
int match1(void *k1, void *k2);
void destroy1(void *key, void *data);

#endif /* HTABLE_H_INCLUDED */
