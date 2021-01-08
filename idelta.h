/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 12:45:35
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-06 14:01:48
 * @Description: 
 */
#ifndef _INCLUDE_IDELTA_H_
#define _INCLUDE_IDELTA_H_

#include "rabin.h"
#include "gear.h"
#include "idedup.h"


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


dedupResPtr dedup_a_single_img(detectionDataPtr detectPtr);
de_dedupPtr de_dedup_a_single_img(de_readPtr decodePtr, jpeg_coe_ptr base);
GHashTable **create_block_index(jpeg_coe_ptr base);

#endif