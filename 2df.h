/*
 * @Author: Cai Deng
 * @Date: 2020-11-19 11:28:26
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-03-03 21:31:13
 * @Description: 
 */
#ifndef _INCLUDE_2DF_H_
#define _INCLUDE_2DF_H_

#include "idedup.h"
#include "buffer.h"
#include "jpeg.h"

extern int SF_NUM, FEA_PER_SF;
#define FEATURE_NUM (SF_NUM*FEA_PER_SF)

typedef struct 
{
    decodedDataPtr  decdData;
    uint64_t        *sfs;
    
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