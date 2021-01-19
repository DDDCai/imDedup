/*
 * @Author: Cai Deng
 * @Date: 2020-11-19 11:28:26
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-19 19:43:18
 * @Description: 
 */
#ifndef _INCLUDE_2DF_H_
#define _INCLUDE_2DF_H_

#include "idedup.h"
#include "buffer.h"
#include "jpeg.h"

#define FEATURE_NUM 16
#define FEA_PER_SF  1
#define SF_NUM      (FEATURE_NUM/FEA_PER_SF)

typedef struct 
{
    decodedDataPtr  decdData;
    uint64_t        sfs[SF_NUM];
    
}   imageData, *imagePtr;

typedef struct detectionInfo
{
    buf_node        *base, *target;
    #ifdef THREAD_OPTI
    GHashTable      **subBlockTab;
    #endif
    uint64_t        mem_size;
    struct detectionInfo    *next;

}   detectionNode, *detectionDataPtr;


void* detect_thread(void *parameter);

#endif