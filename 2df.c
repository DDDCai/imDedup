/*
 * @Author: Cai Deng
 * @Date: 2020-11-19 11:32:09
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-18 20:06:53
 * @Description: 
 */

#include "2df.h"
#include "idelta.h"

uint64_t k_index[16] = {
    0x1a0f3783ef9012db, 0x00a903566bce3501, 0xd2223908bccfe509, 0x5903acde8fd7ab31,
    0x935db607ea31258f, 0xe90788fdac21bd00, 0x235ad90b73c1e502, 0xe547f90ac56b73a2,
    0xa9073451a897d342, 0xc1d23f55690bb5a1, 0x3392b830b514a6f5, 0x6aaa890d35f0ff59,
    0x763fcba8bd62469f, 0x4fdb4529602ad675, 0x8f8263b034fadbc7, 0xf83bd098236ac562
};

uint64_t b_index[16] = {
    0x3764bdca939cad56, 0x290bfd3ea9c74cbe, 0xcb32a05648982795, 0xb2083afde0219374,
    0x09389bfad721f43d, 0x458475badc30a38d, 0xbad72854902bd01a, 0xcf81993a3acb4302,
    0xf4b8eac294a96d54, 0x18321da9c9410111, 0x00df012104bc0103, 0x110018201acdf900,
    0xcc490ab371f1138f, 0x9327ad39875abef4, 0xabbb29843297f091, 0x0932998100000ac0
};

static void free_node(gpointer p)
{
    buf_node    *node   =   (buf_node*)p;
    imagePtr    image   =   (imagePtr)node->data;

    decodedDataPtr  decodeptr   =   image->decdData;

        rawDataPtr  rawPtr  =   decodeptr->rawData;
        free(rawPtr->name);
        free(rawPtr->data);
        free(rawPtr);

            jpeg_coe_ptr    coe =   decodeptr->targetInfo->coe;
            if(coe)
            {
                free(coe->data);
                free(coe);
            }
        free(decodeptr->targetInfo);

    free(decodeptr);

    free(image);
    pthread_mutex_destroy(&node->mutex);
    free(node);
}

static void free_buf_node(void *p)
{
    imagePtr    image   =   (imagePtr)p;
    jpeg_coe_ptr    ptr =   image->decdData->targetInfo->coe;
    free(ptr->data);
    free(ptr);
    image->decdData->targetInfo->coe    =   NULL;
}

static uint64_t fill_buf_node(void *p)
{
    buf_node    *node   =   (buf_node*)p;
    decodedDataPtr  decode  =   ((imagePtr)node->data)->decdData;
    target_ptr  ptr     =   decode->targetInfo;
    if(ptr->coe == NULL)
    {
        ptr->coe    =   get_base_coe_mem(decode->rawData->data, decode->rawData->size);
        return  node->size;
    }
    return 0;
}

static imagePtr compute_features(decodedDataPtr decodePtr)
{
    int         i, j, k, m;
    imagePtr    image   =   (imagePtr)malloc(sizeof(imageData));
    uint32_t    w       =   decodePtr->targetInfo->coe->imgSize[0],
                h       =   decodePtr->targetInfo->coe->imgSize[1];
    uint8_t     map[h+1][w+1];
    JBLOCKROW   jbrow[h];
    uint8_t     *ptr    =   decodePtr->targetInfo->coe->data;
    for(i=0; i<h; i++, ptr+=sizeof(JBLOCK)*w)
        jbrow[i]    =   (JBLOCKROW)ptr;
    JBLOCKARRAY jbarray =   jbrow;

    for(i=0; i<h; i++)
    {
        for(j=0; j<w; j++)
            map[i][j]   =   ((jbarray[i][j][0] + jbarray[i][j][1] + jbarray[i][j][24]) & 2)? 1:0;
    }

    uint64_t    max[FEATURE_NUM] = {0};
    uint64_t    *tmpFeature = (uint64_t*)malloc(sizeof(uint64_t));
    uint8_t     *subFeature = (uint8_t*)tmpFeature;
    uint64_t    ltFeature;

    for(i=0; i<=h-8; i++)
    {
        for(k=0; k<8; k++)
        {
            subFeature[k] = 0;
            for(m=0; m<8; m++)
                subFeature[k] |= (map[i+k][m] << (7-m));
        }
        for(j=0; j<=w-8; j++)
        {
            for(m=0; m<FEATURE_NUM; m++)
            {
                ltFeature = k_index[m]*(*tmpFeature) + b_index[m];
                if(ltFeature > max[m])
                    max[m] = ltFeature;
            }
            for(m=0; m<8; m++)
                subFeature[m] = (subFeature[m] << 1) | map[i+m][j+8];
        }
    }
    free(tmpFeature);

    for(i=0,k=0; i<FEATURE_NUM; k++)
    {
        image->sfs[k] = 0;
        for(j=0; j<FEA_PER_SF; j++,i++)
        {
            image->sfs[k] += max[i];
        }
    }
    image->decdData =   decodePtr;

    return image;
}

#define META_SIZE_2DF (sizeof(buf_node) + sizeof(imageData) + sizeof(decodedDataNode) + sizeof(rawDataNode) + \
    sizeof(target_struct) + sizeof(jpeg_coe) )

static detectionDataPtr detect_a_single_img(decodedDataPtr decodePtr, GHashTable **featureT, Buffer *buf
    #if DETECT_THREAD_NUM!=1
        , pthread_mutex_t *ftMutex
    #endif
)
{
    imagePtr        image   =   compute_features(decodePtr), baseImage;
    uint64_t        *features   =   image->sfs;
    GPtrArray       *bases[SF_NUM], *newArray;
    int             i, j, k;
    uint32_t        tmp;
    uint32_t        matchCounter, bestCounter = 0;
    buf_node        *node   =   (buf_node*)malloc(sizeof(buf_node)), *baseNode, *bestMatch = NULL;

    pthread_mutex_init(&node->mutex, NULL);
    node->data  =   image;
    node->link  =   1;
    // node->size  =   1;
    node->size  =   META_SIZE_2DF + decodePtr->rawData->size;
    for(i=0; i<3; i++)
        node->size += 
            decodePtr->targetInfo->coe->imgSize[2*i]*decodePtr->targetInfo->coe->imgSize[2*i+1]*sizeof(JBLOCK);
    insert_to_buffer(node, buf, free_buf_node);

    for(i=0; i<SF_NUM; i++)
    {
        #if DETECT_THREAD_NUM!=1
        pthread_mutex_lock(ftMutex+i);
        #endif
        
        bases[i]    =   (GPtrArray*)g_hash_table_lookup(featureT[i], features+i);
        if(bestCounter < SF_NUM && bases[i])
        {
            tmp =   bases[i]->len;
            for(j=0; j<tmp; j++)
            {
                baseNode    =   (buf_node*)g_ptr_array_index(bases[i], j);
                baseImage   =   (imagePtr)(baseNode->data);
                for(k=0,matchCounter=0; k<SF_NUM; k++)
                {
                    if(baseImage->sfs[k]==features[k])
                        matchCounter ++;
                }
                if(matchCounter > bestCounter)
                {
                    bestCounter =   matchCounter;
                    bestMatch   =   baseNode;
                }
                if(matchCounter == SF_NUM)
                    break;
            }
        }
        
        if(bases[i])
            g_ptr_array_add(bases[i], node);
        else 
        {
            if(i)   newArray = g_ptr_array_new_full(2, NULL);
            else    newArray = g_ptr_array_new_full(2, free_node);
            g_ptr_array_add(newArray, node);
            g_hash_table_insert(featureT[i], features+i, newArray);
        }

        #if DETECT_THREAD_NUM!=1
        pthread_mutex_unlock(ftMutex+i);
        #endif
    }

    if(bestMatch)
    {
        detectionDataPtr    detectPtr   =   (detectionDataPtr)malloc(sizeof(detectionNode));
        detectPtr->base     =   bestMatch;
        detectPtr->target   =   node;

        pthread_mutex_lock(&bestMatch->mutex);
        bestMatch->link  ++;
        pthread_mutex_unlock(&bestMatch->mutex);
        move_in_buffer(bestMatch, buf, fill_buf_node, free_buf_node);
        
        return  detectPtr;
    }

    pthread_mutex_lock(&node->mutex);
    node->link  --;
    pthread_mutex_unlock(&node->mutex);
    return  NULL;
}

void* detect_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    List        decodeList  =   (List)arg[0];
    List        detectList  =   (List)arg[1];
    char        *outPath    =   (char*)arg[2];
    GHashTable  **featureT  =   (GHashTable**)arg[3];
    pthread_mutex_t *ftMutex=   (pthread_mutex_t*)arg[4];
    Buffer      *decodeBuf  =   (Buffer*)arg[5];
    decodedDataPtr      decodePtr;
    detectionDataPtr    detectPtr;

    uint64_t    *unhandledSize   =   (uint64_t*)g_malloc0(sizeof(uint64_t));

    #ifdef PART_TIME
    GTimer      *timer  =   g_timer_new();
    #endif

    while(1)
    {
        pthread_mutex_lock(&decodeList->mutex);
        while(decodeList->counter == 0)
        {
            if(decodeList->ending == 1) goto ESCAPE_LOOP;
            pthread_cond_wait(&decodeList->rCond, &decodeList->mutex);
        }
        decodePtr   =   decodeList->head;
        decodeList->head    =   NULL;
        decodeList->tail    =   NULL;
        decodeList->counter =   0;
        pthread_cond_signal(&decodeList->wCond);
        pthread_mutex_unlock(&decodeList->mutex);

        while(decodePtr)
        {
            #ifdef PART_TIME
            g_timer_start(timer);
            #endif

            detectPtr   =   detect_a_single_img(decodePtr, featureT, decodeBuf
            #if DETECT_THREAD_NUM!=1
                , ftMutex
            #endif
            );

            #ifdef PART_TIME
            detect_time +=  g_timer_elapsed(timer, NULL);
            #endif

            if(detectPtr)
            {
                #ifdef THREAD_OPTI
                #ifdef PART_TIME
                g_timer_start(timer);
                #endif
                detectPtr->subBlockTab  =   
                    create_block_index(((imagePtr)detectPtr->base->data)->decdData->targetInfo->coe);
                    // create_block_index(detectPtr->base->targetInfo->coe);
                #ifdef PART_TIME
                detect_time +=  g_timer_elapsed(timer, NULL);
                #endif
                #endif

                decodePtr       =   decodePtr->next;
                detectPtr->next =   NULL;
                pthread_mutex_lock(&detectList->mutex);
                if(detectList->counter)
                    ((detectionDataPtr)detectList->tail)->next  =   detectPtr;
                else
                    detectList->head    =   detectPtr;
                detectList->tail    =   detectPtr;
                detectList->counter ++;
                pthread_cond_signal(&detectList->rCond);
                while(detectList->counter == OTHER_LIST_LEN)
                    pthread_cond_wait(&detectList->wCond, &detectList->mutex);
                pthread_mutex_unlock(&detectList->mutex);
            }
            else 
            {
                #ifndef DO_NOT_WRITE
                char    outFilePath[MAX_PATH_LEN];
                PUT_3_STRS_TOGETHER(outFilePath, outPath, "/", decodePtr->rawData->name);
                FILE    *outFp  =   fopen(outFilePath, "wb");
                fwrite(decodePtr->rawData->data, 1, decodePtr->rawData->size, outFp);
                fclose(outFp);
                #endif
                *unhandledSize   +=  decodePtr->rawData->size;
                decodePtr        =   decodePtr->next;
            }
        }
    }

    ESCAPE_LOOP:
    pthread_mutex_unlock(&decodeList->mutex);

    #ifdef PART_TIME
    g_timer_destroy(timer);
    #endif

    pthread_mutex_lock(&detectList->mutex);
    detectList->ending ++;
    pthread_cond_signal(&detectList->rCond);
    pthread_mutex_unlock(&detectList->mutex);

    return  (void*)unhandledSize;
}