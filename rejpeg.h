/*
 * @Author: Cai Deng
 * @Date: 2021-01-14 14:38:31
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-15 14:27:10
 * @Description: 
 */

#ifndef _INCLUDE_REJPEG_H_
#define _INCLUDE_REJPEG_H_

#include "idedup.h"
#include "jpeg.h"
#include "idelta.h"

#define ZIGZAG

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

// rejpegResPtr rejpeg_a_single_img(dedupResPtr dedupPtr);
// short *decode_myjpeg(uint8_t *data);
// uint32_t de_encode_a_single_img(char *outPath, jvirt_barray_ptr *coe,
//     uint8_t *header, uint32_t hedLen, uint8_t ffxx, uint8_t xx
//     #ifdef  CHECK_DECOMPRESS
//         , char *oriFilePath
//     #endif
// );
void* rejpeg_thread(void *parameter);
void* de_decode_thread(void *parameter);

#endif