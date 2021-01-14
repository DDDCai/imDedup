/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 12:45:35
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-14 14:34:34
 * @Description: 
 */
#ifndef _INCLUDE_IDELTA_H_
#define _INCLUDE_IDELTA_H_

#include "rabin.h"
#include "gear.h"
#include "idedup.h"
#include "buffer.h"
#include "2df.h"


#define S_SHORT_UNITY   int8_t
#define U_SHORT_UNITY   uint8_t
#define S_MEDIUM_UNITY  int16_t
#define U_MEDIUM_UNITY  uint16_t
#define S_LONG_UNITY    int32_t
#define U_LONG_UNITY    uint32_t

#define S_UNITY S_MEDIUM_UNITY
#define U_UNITY U_MEDIUM_UNITY

#define COPY_X U_UNITY  //  X.
#define COPY_Y U_UNITY  //  Y.
#define COPY_L U_UNITY  //  L.

#define INSERT_L U_UNITY

#define SUB_THREAD_NUM 8
#define INDEX_THREAD_NUM 16

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
    buf_node    *node;
    struct dedupResult  *next;

}   dedupResNode, *dedupResPtr;

typedef struct de_dedupData
{
    char    *name;
    jvirt_barray_ptr *coe;
    jpeg_coe_ptr    content;
    uint8_t     *oriPtr;
    uint8_t ffxx, xx;
    struct  de_dedupData    *next;

}   de_dedupNode, *de_dedupPtr;

dedupResPtr dedup_a_single_img(detectionDataPtr detectPtr);
de_dedupPtr de_dedup_a_single_img(de_readPtr decodePtr, jpeg_coe_ptr base);
GHashTable **create_block_index(jpeg_coe_ptr base);

#endif