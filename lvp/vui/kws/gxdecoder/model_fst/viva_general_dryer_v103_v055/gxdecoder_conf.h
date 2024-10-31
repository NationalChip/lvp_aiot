#ifndef __GX_DECODER_CONFIG_H__
#define __GX_DECODER_CONFIG_H__

#define UMAP_PROTECT

//以下三个参数和fst文件强相关， 文件中可以读到需要多少
#define FINALS_LENGTH 55
#define MAX_STATES 801
#define MAX_ARCS 3090

#define TMP_ARRAY_SIZE 1000
#define MAX_RESULTS 30

#define POOL_SIZE 1000

/*hash冲突时可往后存放的长度*/
#define UMAP_BUCKET_LEN 4
#define UMAP_TABLE_LEN FINALS_LENGTH

/*hash表长度*/
#define HASH_TABLE_LEN 700
/*hash元素池大小*/
#define HASH_ELEM_SIZE 700

#endif