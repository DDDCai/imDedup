/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 12:50:42
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-15 14:28:18
 * @Description: 
 */
#ifndef _INCLUDE_JPEG_H_
#define _INCLUDE_JPEG_H_

#include "idedup.h"

#define NO_PROGRESSIVE

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

typedef struct decodeData
{
    rawDataPtr  rawData;
    target_ptr  targetInfo;
    struct  decodeData  *next;

}   decodedDataNode, *decodedDataPtr;

jpeg_coe_ptr get_base_coe_mem(uint8_t *data, uint32_t size);
// decodedDataPtr decode_a_single_img(rawDataPtr rawPtr);
void* decode_thread(void *parameter);
void* de_encode_and_write_thread(void *parameter);

#endif