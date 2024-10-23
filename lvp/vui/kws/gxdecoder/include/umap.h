#ifndef __U_MAP_H__
#define __U_MAP_H__

#include "gxdecoder_conf.h"
#pragma pack(4)

typedef struct {
	unsigned short key;
	/*算法中主要使用hash表存float和地址*/
	union {
    	void* addr;
        float f;
    } value;
} KV_NODE;

typedef struct {
	unsigned short id;
	KV_NODE node;
}UMAP_BUCKET_ELEMENT;

typedef struct {
	UMAP_BUCKET_ELEMENT *addr;
} UMAP_ITERATOR;

typedef struct {
	unsigned short cnt;
	UMAP_BUCKET_ELEMENT bucket[UMAP_BUCKET_LEN];
} HASH_BUCKET;

typedef struct {
	int is_init;
	int hash_value;
	int cnt;
	int begin;
	int end;
	int size;
	HASH_BUCKET *map;
	UMAP_ITERATOR *it;
} UMAP;

/********************************************************
* Description : reset hash table
* Parameter ：
* @*p point to UMAP struct
* Return ：0 --success , other -- fail
**********************************************************/
int umap_reset(UMAP *p);

/********************************************************
* Description : init hash table
* Parameter ：
* @*p point to UMAP struct
* Return ：0 --success , other -- fail
**********************************************************/
int umap_init(UMAP *p);

/********************************************************
* Description : insert data(k-v) to hash table
* Parameter ：
* @*p point to UMAP struct
* @v k-v data
* Return ：0 --success , other -- fail
**********************************************************/
int umap_insert(UMAP *p, KV_NODE v);

/********************************************************
* Description : find data in hash table by key
* Parameter ：
* @*p point to UMAP struct
* @key key in k-v data
* Return ：k-v data base --success , NULL -- fail
**********************************************************/
KV_NODE *umap_find(UMAP *p, int key);

/********************************************************
* Description : delete data in hash table by key
* Parameter ：
* @*p point to UMAP struct
* @key key in k-v data
* Return ：0 --success , other -- fail
**********************************************************/
int umap_delete(UMAP *p, int key);

/********************************************************
* Description : uninit the hash table
* Parameter ：
* @*p point to UMAP struct
* Return ：0 --success , other -- fail
**********************************************************/
int umap_uninit(UMAP *p);

void umap_deep_cpy(UMAP *a, UMAP *b);

int umap_get_buffer_size(int table_len);

int umap_set_buffer(UMAP *p, unsigned char *buf, int k);

int get_umap_max_memory(void);
#endif
