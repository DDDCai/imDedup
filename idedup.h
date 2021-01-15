/*
 * @Author: Cai Deng
 * @Date: 2020-11-09 14:22:29
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-15 15:46:36
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
#define DO_NOT_WRITE
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
#define MIDDLE_THREAD_NUM 1
#define WRITE_THREAD_NUM 1

/*------------------------------------------*/

#define DECODE_BUFFER_SIZE (4l<<30)  /* if it represents the image 
/* number, it should be bigger than MIDDLE_THREAD_NUM; or if 
/* it is the absolute space size, it should be bigger than the 
/* size of MIDDLE_THREAD_NUM pieces of images.  */
#define START_TO_MOVE (512l<<20) /* if it represents the image 
/* number, it should be bigger than 1; or if it is the absolute 
/* space size, it should be bigger than the size of one piece 
/* of image.  */

/*------------------------------------------*/

#define END_WITH_FFXX

/*------------------------------------------*/

#define MAX_PATH_LEN 256
#define READ_LIST_LEN 64
#define OTHER_LIST_LEN 64

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
    pthread_mutex_t mutex;
    pthread_cond_t  rCond, wCond;             /* read & write. */
    u_int32_t       counter;
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

/*------------------------------------------*/

uint64_t idedup_decompress(char *inFolder, char *outFolder
    #ifdef  CHECK_DECOMPRESS
        , char *oriFolder
    #endif
);

/*------------------------------------------*/

#endif