/*
 * @Author: Cai Deng
 * @Date: 2020-11-09 14:22:29
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-12 10:33:17
 * @Description: 
 */
#ifndef _INCLUDE_IDEDUP_H_
#define _INCLUDE_IDEDUP_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pthread.h>
#include "fse/lib/fse.h"
#include "glib.h"
#include "stddef.h"
#include "jpeglib.h"
#include <unistd.h>

/*------------------------------------------*/

#define DEBUG_1
#define CHECK_DECOMPRESS
// #define DO_NOT_WRITE
// #define PART_TIME

/*------------------------------------------*/

#define HEADER_DELTA
// #define COMPRESS_DELTA_INS
#define THREAD_OPTI
// #define JPEG_SEPA_COMP

/*------------------------------------------*/

#define USE_RABIN
#define USE_GEAR
#ifdef USE_RABIN
#undef USE_GEAR
#endif

/*------------------------------------------*/

#define READ_THREAD_NUM 1   // DO NOT MODIFY READ_THREAD_NUM !!!
#define DECODE_THREAD_NUM 2
#define DETECT_THREAD_NUM 2
#define DEDUP_THREAD_NUM 2
#define REJPEG_THREAD_NUM 1
#define WRITE_THREAD_NUM 1

/*------------------------------------------*/

#define END_WITH_FFXX

/*------------------------------------------*/

typedef struct 
{
    uint8_t                 *data;
    uint8_t                 *header;
    uint32_t                headerSize;
    uint32_t                imgSize[6];

}   jpeg_coe, *jpeg_coe_ptr;

typedef struct 
{
    jpeg_coe_ptr    coe;
    #ifdef END_WITH_FFXX
    uint8_t         ffxx, xx;
    #endif

}   target_struct, *target_ptr;

/*------------------------------------------*/

#define MAX_PATH_LEN 256
#define READ_LIST_LEN 8192
#define OTHER_LIST_LEN 8192

#define FSE

#define PUT_3_STRS_TOGETHER(des,src1,src2,src3) \
{                                               \
    strcpy(des,src1);                           \
    strcat(des,src2);                           \
    strcat(des,src3);                           \
}

typedef struct 
{
    void            *head, *tail;
    u_int32_t       counter;
    pthread_mutex_t mutex;
    pthread_cond_t  rCond, wCond;             /* read & write. */
    u_int8_t        ending;

}   ListNode, *List;

#define INIT_LIST(list)                       \
{                                             \
    (list)->head  =   NULL;                   \
    (list)->tail  =   NULL;                   \
    (list)->counter   =   0;                  \
    pthread_mutex_init(&(list)->mutex,NULL);  \
    pthread_cond_init(&(list)->rCond,NULL);   \
    pthread_cond_init(&(list)->wCond,NULL);   \
    (list)->ending    =   0;                  \
}

#define DESTROY_LIST(list)                    \
{                                             \
    pthread_mutex_destroy(&(list)->mutex);    \
    pthread_cond_destroy(&(list)->rCond);     \
    pthread_cond_destroy(&(list)->wCond);     \
    free(list);                               \
}

/*------------------------------------------*/

typedef struct rawData 
{
    char        *name;
    u_int8_t    *data;
    u_int32_t   size;
    struct rawData  *next;

}   rawDataNode, *rawDataPtr;

typedef struct decodeData
{
    rawDataPtr  rawData;
    target_ptr  targetInfo;
    struct  decodeData  *next;

}   decodedDataNode, *decodedDataPtr;

typedef struct detectionInfo
{
    decodedDataPtr  base, target;
    #ifdef THREAD_OPTI
    GHashTable      **subBlockTab;
    #endif
    struct detectionInfo    *next;

}   detectionNode, *detectionDataPtr;

typedef struct dedupResult
{
    GArray      *copy_x, *copy_y, *copy_l, *insert_l;
    GArray      *insert_p;
    uint8_t     *header;
    uint32_t    headerSize, y_counter, u_counter, v_counter;
    #ifdef JPEG_SEPA_COMP
    uint32_t    p_counter[3];
    #endif
    char        *baseName, *name;
    uint32_t    imgSize[4];
    uint8_t     ffxx, xx;
    struct dedupResult  *next;

}   dedupResNode, *dedupResPtr;

typedef struct rejpegResult
{
    dedupResPtr dedupRes;
    uint8_t     *rejpegRes;
    uint32_t    rejpegSize;
    #ifdef COMPRESS_DELTA_INS
    uint8_t     flag;
    uint8_t     *cpx, *cpy, *cpl, *inl;
    uint32_t    cpxSize, cpySize, cplSize, inlSize;
    #endif
    struct rejpegResult *next;

}   rejpegResNode, *rejpegResPtr;

/*------------------------------------------*/

uint32_t entropy_compress(void *src, uint32_t srcSize, void *dst, uint32_t dstSize);
uint64_t* idedup_compress(char *inFolder, char *outFolder);

/*------------------------------------------*/

typedef struct de_readData
{
    char        *name, *basename_and_oriptr;
    uint32_t    *sizes;
    uint8_t     *header, *x, *y, *cp_l, *in_l, *in_d;
    uint8_t     ffxx, xx;
    struct  de_readData *next;

}   de_readNode, *de_readPtr;

typedef struct de_dedupData
{
    char    *name;
    jvirt_barray_ptr *coe;
    jpeg_coe_ptr    content;
    uint8_t     *oriPtr;
    uint8_t ffxx, xx;
    struct  de_dedupData    *next;

}   de_dedupNode, *de_dedupPtr;

/*------------------------------------------*/

uint64_t idedup_decompress(char *inFolder, char *outFolder
    #ifdef  CHECK_DECOMPRESS
        , char *oriFolder
    #endif
);

/*------------------------------------------*/

#endif