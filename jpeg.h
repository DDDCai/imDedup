/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 12:50:42
 * @LastEditors: Cai Deng
 * @LastEditTime: 2020-12-09 12:50:36
 * @Description: 
 */
#ifndef _INCLUDE_JPEG_H_
#define _INCLUDE_JPEG_H_

#include "idedup.h"

#define NO_PROGRESSIVE
#define ZIGZAG

jpeg_coe_ptr get_base_coe_mem(uint8_t *data, uint32_t size);
decodedDataPtr decode_a_single_img(rawDataPtr rawPtr);
rejpegResPtr rejpeg_a_single_img(dedupResPtr dedupPtr);
short *decode_myjpeg(uint8_t *data);
uint32_t de_encode_a_single_img(char *outPath, jvirt_barray_ptr *coe,
    uint8_t *header, uint32_t hedLen, uint8_t ffxx, uint8_t xx
    #ifdef  CHECK_DECOMPRESS
        , char *oriFilePath
    #endif
);

#endif