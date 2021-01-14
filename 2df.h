/*
 * @Author: Cai Deng
 * @Date: 2020-11-19 11:28:26
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-14 11:34:31
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
    // decodedDataPtr  base, target;
    buf_node        *base, *target;
    #ifdef THREAD_OPTI
    GHashTable      **subBlockTab;
    #endif
    struct detectionInfo    *next;

}   detectionNode, *detectionDataPtr;

detectionDataPtr detect_a_single_img(decodedDataPtr decodePtr, GHashTable **featureT
    , Buffer *buf
    #if DETECT_THREAD_NUM!=1
        , pthread_mutex_t *ftMutex
    #endif
);

#endif