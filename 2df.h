/*
 * @Author: Cai Deng
 * @Date: 2020-11-19 11:28:26
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-11 22:29:28
 * @Description: 
 */
#ifndef _INCLUDE_2DF_H_
#define _INCLUDE_2DF_H_

#include "idedup.h"

#define FEATURE_NUM 16
#define FEA_PER_SF  1
#define SF_NUM      (FEATURE_NUM/FEA_PER_SF)

typedef struct 
{
    decodedDataPtr  decdData;
    uint64_t        sfs[SF_NUM];
    
}   imageData, *imagePtr;

detectionDataPtr detect_a_single_img(decodedDataPtr decodePtr, GHashTable **featureT
    #if DETECT_THREAD_NUM!=1
        , pthread_mutex_t *ftMutex
    #endif
);

#endif