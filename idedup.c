/*
 * @Author: Cai Deng
 * @Date: 2020-11-09 14:24:32
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-14 14:42:10
 * @Description: 
 */
#include "idedup.h"
#include "2df.h"
#include "idelta.h"
#include "jpeg.h"
#include "rejpeg.h"
#include "buffer.h"

#ifdef PART_TIME
extern double read_time;
extern double decode_time;
extern double detect_time;
extern double dedup_time;
extern double rejpeg_time;
extern double write_time;
#endif

uint32_t entropy_compress(void *src, uint32_t srcSize, void *dst, uint32_t dstSize)
{
    uint64_t   returnVal   =   
    #ifdef  HUFFMAN
        HUF_compress
    #endif
    #ifdef   FSE
        FSE_compress
    #endif
        (dst,dstSize,src,srcSize);

    if(returnVal==0 || 
        #ifdef  HUFFMAN
            HUF_isError
        #endif
        #ifdef  FSE
            FSE_isError
        #endif
            (returnVal)
    )
        return 0;
    return returnVal;
}

/*---------------------------------------------------------------------*/

static void* read_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    char        *folderPath =   (char*)arg[0];
    List        rawList     =   (List)arg[1];
    DIR         *dir;
    struct      dirent      *entry;
    struct      stat        statbuf;
    char        filePath[MAX_PATH_LEN];
    u_int64_t   fileSize;
    FILE        *fp;
    u_int8_t    *rawDataBuffer;
    char        *nameTmp;
    rawDataPtr  rawTmp;
    u_int64_t   *rawSize    =   (u_int64_t*)g_malloc0(sizeof(u_int64_t));
    #ifdef PART_TIME
    GTimer      *timer      =   g_timer_new();
    #endif

    if(!(dir = opendir(folderPath)))
    {
        printf("Fail to open %s\n",folderPath);
        exit(EXIT_FAILURE);
    }
    while(entry = readdir(dir))
    {
        #ifdef PART_TIME
        g_timer_start(timer);
        #endif

        if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
            continue;
        PUT_3_STRS_TOGETHER(filePath,folderPath,"/",entry->d_name);

        stat(filePath,&statbuf);
        fileSize    =   statbuf.st_size;
        rawDataBuffer   =   (u_int8_t*)malloc(fileSize);
        fp = fopen(filePath,"rb");
        if(fileSize != fread(rawDataBuffer,1,fileSize,fp))
        {
            printf("fail to read %s\n",filePath);
            continue;
        }
        fclose(fp);

        nameTmp =   (char*)malloc(strlen(entry->d_name) + 1);
        strcpy(nameTmp,entry->d_name);
        rawTmp  =   (rawDataPtr)malloc(sizeof(rawDataNode));
        rawTmp->data    =   rawDataBuffer;
        rawTmp->name    =   nameTmp;
        rawTmp->size    =   fileSize;
        rawTmp->next    =   NULL;

        #ifdef PART_TIME
        read_time   +=  g_timer_elapsed(timer, NULL);
        #endif

        pthread_mutex_lock(&rawList->mutex);
        if(rawList->head)
            ((rawDataPtr)rawList->tail)->next   =   rawTmp;
        else
            rawList->head   =   rawTmp;
        rawList->tail   =   rawTmp;
        rawList->counter    ++;
        pthread_cond_signal(&rawList->rCond);
        while(rawList->counter == READ_LIST_LEN)
            pthread_cond_wait(&rawList->wCond,&rawList->mutex);
        pthread_mutex_unlock(&rawList->mutex);

        *rawSize +=  fileSize;
    }
    closedir(dir);

    #ifdef PART_TIME
    g_timer_destroy(timer);
    #endif

    pthread_mutex_lock(&rawList->mutex);
    rawList->ending ++;
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_cond_signal(&rawList->rCond);
    pthread_mutex_unlock(&rawList->mutex);

    return  (void*)rawSize;
}

static void* decode_thread(void *parameter)
{
    void    **arg       =       (void**)parameter;
    rawDataPtr          rawPtr, rawTmp;
    decodedDataPtr      decPtr;
    char    *outPath    =       (char*)arg[0];
    List    rawList     =       (List)arg[1];
    List    decList     =       (List)arg[2];
    char    filePath[MAX_PATH_LEN];
    FILE    *fp;
    u_int64_t       *undecSize  =   (u_int64_t*)g_malloc0(sizeof(u_int64_t));
    #ifdef PART_TIME
    GTimer  *timer      =   g_timer_new();
    #endif

    while(1)
    {
        pthread_mutex_lock(&rawList->mutex);
        while(rawList->counter == 0)
        {
            if(rawList->ending == READ_THREAD_NUM) goto ESCAPE_LOOP;
            pthread_cond_wait(&rawList->rCond, &rawList->mutex);
        }
        rawPtr  =   rawList->head;
        rawList->head   =   NULL;
        rawList->tail   =   NULL;
        rawList->counter    =   0;
        for(int i=0; i<READ_THREAD_NUM; i++)
            pthread_cond_signal(&rawList->wCond);
        pthread_mutex_unlock(&rawList->mutex);

        while(rawPtr)
        {
            #ifdef PART_TIME
            g_timer_start(timer);
            #endif

            decPtr  =   decode_a_single_img(rawPtr);

            #ifdef PART_TIME
            decode_time +=  g_timer_elapsed(timer, NULL);
            #endif

            if(decPtr)
            {
                decPtr->next    =   NULL;
                pthread_mutex_lock(&decList->mutex);
                if(decList->counter)
                    ((decodedDataPtr)decList->tail)->next   =   decPtr;
                else
                    decList->head   =   decPtr;
                decList->tail   =   decPtr;
                decList->counter    ++;

                pthread_cond_signal(&decList->rCond);
                while(decList->counter == OTHER_LIST_LEN)
                    pthread_cond_wait(&decList->wCond,&decList->mutex);
                pthread_mutex_unlock(&decList->mutex);

                rawPtr = rawPtr->next;
            }
            else
            {
                #ifndef DO_NOT_WRITE
                PUT_3_STRS_TOGETHER(filePath,outPath,"/",rawPtr->name);
                fp  =   fopen(filePath,"wb");
                fwrite(rawPtr->data,1,rawPtr->size,fp);
                fclose(fp);
                #endif
                *undecSize  +=  rawPtr->size;
                free(rawPtr->name);
                free(rawPtr->data);
                rawTmp = rawPtr;
                rawPtr = rawPtr->next;
                free(rawTmp);
            }
        }
    }

    ESCAPE_LOOP:
    pthread_mutex_unlock(&rawList->mutex);

    #ifdef PART_TIME
    g_timer_destroy(timer);
    #endif

    pthread_mutex_lock(&decList->mutex);
    decList->ending ++;
    pthread_cond_signal(&decList->rCond);
    pthread_mutex_unlock(&decList->mutex);

    return (void*)undecSize;
}

static void* detect_thread(void *parameter)
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
            }

            decodePtr   =   decodePtr->next;
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

static void* dedup_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    List        detectList  =   (List)arg[0];
    List        dedupList   =   (List)arg[1];
    detectionDataPtr    detectPtr, detectTmp;
    dedupResPtr dedupPtr;
    #ifdef PART_TIME
    GTimer      *timer      =   g_timer_new();
    #endif

    while(1)
    {
        pthread_mutex_lock(&detectList->mutex);
        while(detectList->counter == 0)
        {
            if(detectList->ending == 1) goto ESCAPE_LOOP;
            pthread_cond_wait(&detectList->rCond, &detectList->mutex);
        }
        detectPtr   =   detectList->head;
        detectList->head    =   NULL;
        detectList->tail    =   NULL;
        detectList->counter =   0;
        pthread_cond_signal(&detectList->wCond);
        pthread_mutex_unlock(&detectList->mutex);

        while(detectPtr)
        {
            #ifdef PART_TIME
            g_timer_start(timer);
            #endif

            dedupPtr    =   dedup_a_single_img(detectPtr);
            pthread_mutex_lock(&detectPtr->base->mutex);
            detectPtr->base->link   --;
            pthread_mutex_unlock(&detectPtr->base->mutex);

            #ifdef PART_TIME
            dedup_time  +=  g_timer_elapsed(timer, NULL);
            #endif

            dedupPtr->next  =   NULL;
            pthread_mutex_lock(&dedupList->mutex);
            if(dedupList->counter)
                ((dedupResPtr)dedupList->tail)->next    =   dedupPtr;
            else 
                dedupList->head =   dedupPtr;
            dedupList->tail =   dedupPtr;
            dedupList->counter ++;

            pthread_cond_signal(&dedupList->rCond);
            while(dedupList->counter == OTHER_LIST_LEN)
                pthread_cond_wait(&dedupList->wCond, &dedupList->mutex);
            pthread_mutex_unlock(&dedupList->mutex);

            detectTmp   =   detectPtr;
            detectPtr   =   detectPtr->next;
            free(detectTmp);
        }
    }

    ESCAPE_LOOP:
    pthread_mutex_unlock(&detectList->mutex);

    #ifdef PART_TIME
    g_timer_destroy(timer);
    #endif

    pthread_mutex_lock(&dedupList->mutex);
    dedupList->ending   ++;
    pthread_cond_signal(&dedupList->rCond);
    pthread_mutex_unlock(&dedupList->mutex);
}

#ifdef COMPRESS_DELTA_INS
static void compress_delta_ins(rejpegResPtr rejpeg)
{
    uint32_t    cpxSize =   rejpeg->dedupRes->copy_x->len*sizeof(COPY_X),
                cpySize =   rejpeg->dedupRes->copy_y->len*sizeof(COPY_Y),
                cplSize =   rejpeg->dedupRes->copy_l->len*sizeof(COPY_L),
                inlSize =   rejpeg->dedupRes->insert_l->len*sizeof(INSERT_L);
    rejpeg->cpx =   (uint8_t*)malloc(cpxSize);
    rejpeg->cpy =   (uint8_t*)malloc(cpySize);
    rejpeg->cpl =   (uint8_t*)malloc(cplSize);
    rejpeg->inl =   (uint8_t*)malloc(inlSize);

    rejpeg->cpxSize =   entropy_compress(rejpeg->dedupRes->copy_x->data, cpxSize, rejpeg->cpx, cpxSize);
    if(rejpeg->cpxSize > 0)
        rejpeg->flag    =   0x08;
    else 
    {
        rejpeg->flag    =   0;
        free(rejpeg->cpx);
        rejpeg->cpx     =   NULL;
        rejpeg->cpxSize =   cpxSize;
    }
    rejpeg->cpySize =   entropy_compress(rejpeg->dedupRes->copy_y->data, cpySize, rejpeg->cpy, cpySize);
    if(rejpeg->cpySize > 0)
        rejpeg->flag    |=  0x04;
    else 
    {
        free(rejpeg->cpy);
        rejpeg->cpy     =   NULL;
        rejpeg->cpySize =   cpySize;
    }
    rejpeg->cplSize =   entropy_compress(rejpeg->dedupRes->copy_l->data, cplSize, rejpeg->cpl, cplSize);
    if(rejpeg->cplSize > 0)
        rejpeg->flag    |=  0x02;
    else 
    {
        free(rejpeg->cpl);
        rejpeg->cpl     =   NULL;
        rejpeg->cplSize =   cplSize;
    }
    rejpeg->inlSize =   entropy_compress(rejpeg->dedupRes->insert_l->data, inlSize, rejpeg->inl, inlSize);
    if(rejpeg->inlSize > 0)
        rejpeg->flag    |=  0x01;
    else 
    {
        free(rejpeg->inl);
        rejpeg->inl     =   NULL;
        rejpeg->inlSize =   inlSize;
    }
}
#endif

static void* rejpeg_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    List        dedupList   =   (List)arg[0];
    List        rejpegList  =   (List)arg[1];
    char        *outPath    =   (char*)arg[2];
    dedupResPtr dedupPtr;
    rejpegResPtr    rejpegPtr;
    #ifdef PART_TIME
    GTimer      *timer      =   g_timer_new();
    #endif

    while(1)
    {
        pthread_mutex_lock(&dedupList->mutex);
        while(dedupList->counter == 0)
        {
            if(dedupList->ending == 1) goto ESCAPE_LOOP;
            pthread_cond_wait(&dedupList->rCond, &dedupList->mutex);
        }
        dedupPtr = dedupList->head;
        dedupList->head = NULL;
        dedupList->tail = NULL;
        dedupList->counter = 0;
        pthread_cond_signal(&dedupList->wCond);
        pthread_mutex_unlock(&dedupList->mutex);

        while(dedupPtr)
        {
            #ifdef PART_TIME
            g_timer_start(timer);
            #endif

            rejpegPtr   =   rejpeg_a_single_img(dedupPtr);
            pthread_mutex_lock(&dedupPtr->node->mutex);
            dedupPtr->node->link --;
            pthread_mutex_unlock(&dedupPtr->node->mutex);
            #ifdef COMPRESS_DELTA_INS
            compress_delta_ins(rejpegPtr);
            #endif

            #ifdef PART_TIME
            rejpeg_time +=  g_timer_elapsed(timer, NULL);
            #endif

            rejpegPtr->next =   NULL;
            pthread_mutex_lock(&rejpegList->mutex);
            if(rejpegList->counter)
                ((rejpegResPtr)rejpegList->tail)->next  =   rejpegPtr;
            else 
                rejpegList->head    =   rejpegPtr;
            rejpegList->tail    =   rejpegPtr;
            rejpegList->counter ++;

            pthread_cond_signal(&rejpegList->rCond);
            while(rejpegList->counter == OTHER_LIST_LEN)
                pthread_cond_wait(&rejpegList->wCond, &rejpegList->mutex);
            pthread_mutex_unlock(&rejpegList->mutex);

            dedupPtr    =   dedupPtr->next;
        }
    }

    ESCAPE_LOOP:
    pthread_mutex_unlock(&dedupList->mutex);

    #ifdef PART_TIME
    g_timer_destroy(timer);
    #endif

    pthread_mutex_lock(&rejpegList->mutex);
    rejpegList->ending  ++;
    for(int i=0; i<WRITE_THREAD_NUM; i++)
        pthread_cond_signal(&rejpegList->rCond);
    pthread_mutex_unlock(&rejpegList->mutex);
}

static void* write_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    List        rejpegList  =   (List)arg[0];
    char        *outPath    =   (char*)arg[1];
    uint64_t    *finalSize  =   (uint64_t*)g_malloc0(sizeof(uint64_t));
    char        outFilePath[MAX_PATH_LEN];
    dedupResPtr dedupPtr;
    FILE        *fp;
    rejpegResPtr    rejpegPtr, rejpegTmp;

    #ifdef DEBUG_1
    free(finalSize);
    finalSize   =   (uint64_t*)g_malloc0(sizeof(uint64_t)*9);
    #endif

    #ifdef PART_TIME
    GTimer      *timer  =   g_timer_new();
    #endif

    while(1)
    {
        pthread_mutex_lock(&rejpegList->mutex);
        while(rejpegList->counter == 0)
        {
            if(rejpegList->ending == MIDDLE_THREAD_NUM) goto ESCAPE_LOOP;
            pthread_cond_wait(&rejpegList->rCond, &rejpegList->mutex);
        }
        rejpegPtr   =   rejpegList->head;
        rejpegList->head    =   NULL;
        rejpegList->tail    =   NULL;
        rejpegList->counter =   0;
        for(int i=0; i<MIDDLE_THREAD_NUM; i++)
            pthread_cond_signal(&rejpegList->wCond);
        pthread_mutex_unlock(&rejpegList->mutex);

        while(rejpegPtr)
        {
            #ifdef PART_TIME
            g_timer_start(timer);
            #endif

            dedupPtr    =   rejpegPtr->dedupRes;
            uint32_t    subSize[]   =   {
                dedupPtr->imgSize[0], 
                dedupPtr->imgSize[1], 
                dedupPtr->imgSize[2], 
                dedupPtr->imgSize[3],
                dedupPtr->y_counter, 
                dedupPtr->u_counter, 
                dedupPtr->v_counter,
                dedupPtr->headerSize, 
                #ifdef COMPRESS_DELTA_INS
                rejpegPtr->cpxSize,
                rejpegPtr->cpySize,
                rejpegPtr->cplSize,
                rejpegPtr->inlSize,
                #else
                dedupPtr->copy_x->len*sizeof(COPY_X), 
                dedupPtr->copy_y->len*sizeof(COPY_Y),
                dedupPtr->copy_l->len*sizeof(COPY_L), 
                dedupPtr->insert_l->len*sizeof(INSERT_L), 
                #endif
                rejpegPtr->rejpegSize
            };
            PUT_3_STRS_TOGETHER(outFilePath, outPath, "/", dedupPtr->name);
            strcat(outFilePath, ".sid");

            #ifndef DO_NOT_WRITE
            fp  =   fopen(outFilePath, "wb");
            fwrite(dedupPtr->baseName,      1, strlen(dedupPtr->baseName)+1, fp);
            fwrite(subSize,                 1, sizeof(subSize), fp);
            fwrite(&dedupPtr->ffxx,         1, 1,               fp);
            fwrite(&dedupPtr->xx,           1, 1,               fp);
            fwrite(dedupPtr->header,        1, subSize[7],      fp);
            #ifdef COMPRESS_DELTA_INS
            fwrite(&rejpegPtr->flag,        1, 1,               fp);
            if(rejpegPtr->cpx)
            {
                fwrite(rejpegPtr->cpx,      1, subSize[8],      fp);
                free(rejpegPtr->cpx);
            }
            else 
                fwrite(dedupPtr->copy_x->data,  1,  subSize[8], fp);
            if(rejpegPtr->cpy)
            {
                fwrite(rejpegPtr->cpy,      1, subSize[9],      fp);
                free(rejpegPtr->cpy);
            }
            else 
                fwrite(dedupPtr->copy_y->data,  1,  subSize[9], fp);
            if(rejpegPtr->cpl)
            {
                fwrite(rejpegPtr->cpl,      1, subSize[10],     fp);
                free(rejpegPtr->cpl);
            }
            else 
                fwrite(dedupPtr->copy_l->data,  1,  subSize[10],fp);
            if(rejpegPtr->inl)
            {
                fwrite(rejpegPtr->inl,      1, subSize[11],     fp);
                free(rejpegPtr->inl);
            }
            else 
                fwrite(dedupPtr->insert_l->data, 1, subSize[11],fp);
            #else
            fwrite(dedupPtr->copy_x->data,  1, subSize[8],      fp);
            fwrite(dedupPtr->copy_y->data,  1, subSize[9],      fp);
            fwrite(dedupPtr->copy_l->data,  1, subSize[10],     fp);
            fwrite(dedupPtr->insert_l->data,1, subSize[11],     fp);
            #endif
            fwrite(rejpegPtr->rejpegRes,    1, subSize[12],     fp);
            fclose(fp);
            #endif

            *finalSize += (
                strlen(dedupPtr->baseName)+1+
                sizeof(subSize)+
                2+
                subSize[7]+
                #ifdef COMPRESS_DELTA_INS
                1+
                #endif
                subSize[8]+
                subSize[9]+
                subSize[10]+
                subSize[11]+
                subSize[12]
            );

            #ifdef DEBUG_1
            finalSize[1] +=  (strlen(dedupPtr->baseName)+3+sizeof(subSize))
                                #ifdef COMPRESS_DELTA_INS
                                +   1
                                #endif
                                ;
            finalSize[2] +=  subSize[7];
            finalSize[3] +=  subSize[8];
            finalSize[4] +=  subSize[9];
            finalSize[5] +=  subSize[10];
            finalSize[6] +=  subSize[11];
            finalSize[7] +=  subSize[12];
            finalSize[8] =   finalSize[0];
            #endif

            g_array_free(dedupPtr->copy_x, TRUE);
            g_array_free(dedupPtr->copy_y, TRUE);
            g_array_free(dedupPtr->copy_l, TRUE);
            g_array_free(dedupPtr->insert_l, TRUE);
            g_array_free(dedupPtr->insert_p, TRUE);
            #ifdef HEADER_DELTA
            free(dedupPtr->header);
            #endif
            free(dedupPtr);
            free(rejpegPtr->rejpegRes);
            rejpegTmp   =   rejpegPtr;
            rejpegPtr   =   rejpegPtr->next;
            free(rejpegTmp);

            #ifdef PART_TIME
            write_time  +=  g_timer_elapsed(timer, NULL);
            #endif
        }
    }

    ESCAPE_LOOP:
    pthread_mutex_unlock(&rejpegList->mutex);

    #ifdef PART_TIME
    g_timer_destroy(timer);
    #endif

    return  (void*)finalSize;
}

static void free_ht_val(gpointer p)
{
    g_ptr_array_free(p, TRUE);
}

uint64_t* idedup_compress(char *inFolder, char *outFolder)
{
    Buffer      decodeBuffer;
    decodeBuffer.head   =   NULL;
    decodeBuffer.tail   =   NULL;
    decodeBuffer.size   =   DECODE_BUFFER_SIZE;
    pthread_mutex_init(&decodeBuffer.mutex, NULL);

    #ifdef DEBUG_1
    uint64_t    *result =   (uint64_t*)g_malloc0(sizeof(uint64_t)*11);
    #else
    uint64_t    *result =   (uint64_t*)g_malloc0(sizeof(uint64_t)*3);
    #endif
    List        rawList =   (List)malloc(sizeof(ListNode));
    List        decList[MIDDLE_THREAD_NUM], detList[MIDDLE_THREAD_NUM], dupList[MIDDLE_THREAD_NUM];
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
    {
        decList[i] = (List)malloc(sizeof(ListNode));
        detList[i] = (List)malloc(sizeof(ListNode));
        dupList[i] = (List)malloc(sizeof(ListNode));
    }
    List        rejList =   (List)malloc(sizeof(ListNode));

    INIT_LIST(rawList);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
    {
        INIT_LIST(decList[i]);
        INIT_LIST(detList[i]);
        INIT_LIST(dupList[i]);
    }
    INIT_LIST(rejList);

    GHashTable  *featureT[SF_NUM];
    pthread_mutex_t ftMutex[SF_NUM];
    for(int i=0; i<SF_NUM; i++)
    {
        featureT[i] =   g_hash_table_new_full(g_int64_hash, g_int64_equal, NULL, free_ht_val);
        pthread_mutex_init(&ftMutex[i], NULL);
    }

    pthread_t   read_t_id[READ_THREAD_NUM], decd_t_id[MIDDLE_THREAD_NUM], detc_t_id[MIDDLE_THREAD_NUM], 
                dedup_t_id[MIDDLE_THREAD_NUM], rejpg_t_id[MIDDLE_THREAD_NUM], writ_t_id[WRITE_THREAD_NUM];

    void        *read_arg[] = {inFolder, rawList};
    void        **decd_arg[MIDDLE_THREAD_NUM], **detc_arg[MIDDLE_THREAD_NUM], **dedu_arg[MIDDLE_THREAD_NUM], **reje_arg[MIDDLE_THREAD_NUM];
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
    {
        decd_arg[i] = (void**)malloc(sizeof(void*)*3);
        detc_arg[i] = (void**)malloc(sizeof(void*)*6);
        dedu_arg[i] = (void**)malloc(sizeof(void*)*2);
        reje_arg[i] = (void**)malloc(sizeof(void*)*3);
        decd_arg[i][0] = outFolder;
        decd_arg[i][1] = rawList;
        decd_arg[i][2] = decList[i];
        detc_arg[i][0] = decList[i];
        detc_arg[i][1] = detList[i];
        detc_arg[i][2] = outFolder;
        detc_arg[i][3] = featureT;
        detc_arg[i][4] = ftMutex;
        detc_arg[i][5] = &decodeBuffer;
        dedu_arg[i][0] = detList[i];
        dedu_arg[i][1] = dupList[i];
        reje_arg[i][0] = dupList[i];
        reje_arg[i][1] = rejList;
        reje_arg[i][2] = outFolder;
    }
    void        *writ_arg[] = {rejList, outFolder};

    void        *rawSize[READ_THREAD_NUM], *undecdSize[MIDDLE_THREAD_NUM], 
                *finalSize[WRITE_THREAD_NUM], *unhandledSize[MIDDLE_THREAD_NUM];

    for(int i=0; i<READ_THREAD_NUM; i++)
        pthread_create(&read_t_id[i], NULL, read_thread, (void*)read_arg);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_create(&decd_t_id[i], NULL, decode_thread, (void*)decd_arg[i]);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_create(&detc_t_id[i], NULL, detect_thread, (void*)detc_arg[i]);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_create(&dedup_t_id[i], NULL, dedup_thread, (void*)dedu_arg[i]);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_create(&rejpg_t_id[i], NULL, rejpeg_thread, (void*)reje_arg[i]);
    for(int i=0; i<WRITE_THREAD_NUM; i++)
        pthread_create(&writ_t_id[i], NULL ,write_thread, (void*)writ_arg);

    for(int i=0; i<READ_THREAD_NUM; i++)
        pthread_join(read_t_id[i], (void**)(&rawSize[i]));
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_join(decd_t_id[i], (void**)(&undecdSize[i]));
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_join(detc_t_id[i], (void**)(&unhandledSize[i]));
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_join(dedup_t_id[i], NULL);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_join(rejpg_t_id[i], NULL);
    for(int i=0; i<WRITE_THREAD_NUM; i++)
        pthread_join(writ_t_id[i], (void**)(&finalSize[i]));

    for(int i=0; i<SF_NUM; i++)
    {
        g_hash_table_destroy(featureT[i]);
        pthread_mutex_destroy(&ftMutex[i]);
    }

    pthread_mutex_destroy(&decodeBuffer.mutex);

    for(int i=0; i<READ_THREAD_NUM; i++)
        result[0] += *((uint64_t*)rawSize[i]);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        result[1] += *((uint64_t*)undecdSize[i]);
    for(int i=0; i<WRITE_THREAD_NUM; i++)
        result[2] += *((uint64_t*)finalSize[i]);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        result[2] += *((uint64_t*)unhandledSize[i]);
    #ifdef DEBUG_1
    for(int j=0; j<WRITE_THREAD_NUM; j++)
        for(int i=0; i<8; i++)
            result[3+i] += ((uint64_t*)finalSize[j])[1+i];
    #endif

    for(int i=0; i<READ_THREAD_NUM; i++)
        free(rawSize[i]);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        free(undecdSize[i]);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        free(unhandledSize[i]);
    for(int i=0; i<WRITE_THREAD_NUM; i++)
        free(finalSize[i]);
    
    DESTROY_LIST(rawList);
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
    {
        DESTROY_LIST(decList[i]);
        DESTROY_LIST(detList[i]);
        DESTROY_LIST(dupList[i]);
        free(decd_arg[i]);
        free(detc_arg[i]);
        free(dedu_arg[i]);
        free(reje_arg[i]);
    }
    DESTROY_LIST(rejList);

    return result;
}

/*---------------------------------------------------------------------*/

#define CHECK_POSTFIX(fileName,lastName)  (!strcmp(lastName,(fileName) + strlen((fileName)) - strlen((lastName))))

void* de_read_thread(void *parameter)
{
    void        **arg   =   (void**)parameter;
    char        *inPath =   (char*)arg[0];
    List        readList=   (List)arg[1];
    List        tabList =   (List)arg[2];
    GHashTable  *coeTab =   (GHashTable*)tabList->head;
    pthread_mutex_t mutex  =   tabList->mutex;
    struct      stat    stbuf;
    DIR         *dir;
    struct      dirent  *entry;
    FILE        *fp;
    char        *name;
    char        filePath[MAX_PATH_LEN];
    uint8_t     *buf, *ptr;
    de_readPtr  readData;
    jpeg_coe_ptr    coe;
    uint64_t    *returnSize =   (uint64_t*)g_malloc0(sizeof(uint64_t));

    if(!(dir = opendir(inPath)))
    {
        printf("fail to open folder %s\n", inPath);
        exit(EXIT_FAILURE);
    }
    while(entry = readdir(dir))
    {
        name    =   (char*)malloc(MAX_PATH_LEN);
        strcpy(name, entry->d_name);
        PUT_3_STRS_TOGETHER(filePath, inPath, "/", entry->d_name);
        stat(filePath, &stbuf);
        fp  =   fopen(filePath, "rb");
        buf =   (uint8_t*)malloc(stbuf.st_size);
        if(stbuf.st_size != fread(buf, 1, stbuf.st_size, fp)) ;
        fclose(fp);

        if(CHECK_POSTFIX(entry->d_name, "sid"))
        {
            readData    =   (de_readPtr)malloc(sizeof(de_readNode));
            name[strlen(name)-4] = '\0';
            readData->name  =   name;
            ptr =   buf;
            readData->basename_and_oriptr   =   ptr;
            ptr +=  (strlen(readData->basename_and_oriptr) + 1);
            readData->sizes  =   (uint32_t*)ptr;
            ptr +=  13*sizeof(uint32_t);
            readData->ffxx  =   *ptr++;
            readData->xx    =   *ptr++;
            readData->header    =   ptr;
            ptr +=  readData->sizes[7];
            readData->x =   ptr;
            ptr +=  readData->sizes[8];
            readData->y =   ptr;
            ptr +=  readData->sizes[9];
            readData->cp_l  =   ptr;
            ptr +=  readData->sizes[10];
            readData->in_l  =   ptr;
            ptr +=  readData->sizes[11];
            readData->in_d  =   ptr;
            readData->next  =   NULL;

            pthread_mutex_lock(&readList->mutex);
            if(readList->head)
                ((de_readPtr)readList->tail)->next  =   readData;
            else 
                readList->head  =   readData;
            readList->tail  =   readData;
            readList->counter   ++;
            pthread_cond_signal(&readList->rCond);
            if(readList->counter == READ_LIST_LEN)
                pthread_cond_wait(&readList->wCond, &readList->mutex);
            pthread_mutex_unlock(&readList->mutex);
        }
        else if(CHECK_POSTFIX(entry->d_name, "jpg") || CHECK_POSTFIX(entry->d_name, "jpeg"))
        {
            coe =   get_base_coe_mem(buf, stbuf.st_size);
            if(coe)
            {
                pthread_mutex_lock(&mutex);
                g_hash_table_insert(coeTab, name, coe);
                pthread_mutex_unlock(&mutex);
                *returnSize +=  stbuf.st_size;
            }
            #ifndef HEADER_DELTA
            free(buf);
            #endif
        }
        else 
        {
            free(name);
            free(buf);
        }
    }
    closedir(dir);

    pthread_mutex_lock(&readList->mutex);
    readList->ending    =   1;
    pthread_mutex_unlock(&readList->mutex);
    pthread_cond_signal(&readList->rCond);

    return (void*)returnSize;
}

void* de_decode_thread(void *parameter)
{
    void        **arg    =   (void**)parameter;
    List        readList =   (List)arg[0];
    List        decdList =   (List)arg[1];
    de_readPtr  readPtr, readTmp;

    while(1)
    {
        pthread_mutex_lock(&readList->mutex);
        if(readList->counter == 0)
        {
            if(readList->ending)  break;
            else 
            {
                pthread_cond_wait(&readList->rCond, &readList->mutex);
                if(readList->counter == 0)    break;
            }
        }
        readPtr   =   readList->head;
        readList->head    =   NULL;
        readList->tail    =   NULL;
        readList->counter =   0;
        pthread_mutex_unlock(&readList->mutex);
        pthread_cond_signal(&readList->wCond);

        while(readPtr)
        {
            if(readPtr->sizes[12])
                readPtr->in_d =   (uint8_t*)decode_myjpeg(readPtr->in_d);
            else 
                readPtr->in_d =   NULL;

            readTmp =   readPtr->next;
            readPtr->next   =   NULL;
            pthread_mutex_lock(&decdList->mutex);
            if(decdList->counter)
                ((de_readPtr)decdList->tail)->next  =   readPtr;
            else 
                decdList->head  =   readPtr;
            decdList->tail  =   readPtr;
            decdList->counter   ++;
            pthread_cond_signal(&decdList->rCond);
            if(decdList->counter == OTHER_LIST_LEN)
                pthread_cond_wait(&decdList->wCond, &decdList->mutex);
            pthread_mutex_unlock(&decdList->mutex);

            readPtr =   readTmp;
        }
    }

    pthread_mutex_lock(&decdList->mutex);
    decdList->ending = 1;
    pthread_mutex_unlock(&decdList->mutex);
    pthread_cond_signal(&decdList->rCond);
}

void* de_dedup_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    List        decodeList  =   (List)arg[0];
    List        dedupList   =   (List)arg[1];
    List        tabList     =   (List)arg[2];
    GHashTable  *coeTable   =   (GHashTable*)tabList->head;
    pthread_mutex_t mutex   =   tabList->mutex;
    de_readPtr  decodePtr   =   decodeList->head, decodeTail    =   decodeList->tail, decodeTmp;
    de_dedupPtr dedupPtr;
    jpeg_coe_ptr    base;

    while(decodePtr)
    {
        pthread_mutex_lock(&mutex);
        base    =   g_hash_table_lookup(coeTable, decodePtr->basename_and_oriptr);
        pthread_mutex_unlock(&mutex);
        
        if(base)
        {
            dedupPtr    =   de_dedup_a_single_img(decodePtr, base);
            pthread_mutex_lock(&mutex);
            g_hash_table_insert(coeTable,dedupPtr->name,dedupPtr->content);
            pthread_mutex_unlock(&mutex);

            dedupPtr->next  =   NULL;
            pthread_mutex_lock(&dedupList->mutex);
            if(dedupList->counter)
                ((de_dedupPtr)dedupList->tail)->next    =   dedupPtr;
            else 
                dedupList->head =   dedupPtr;
            dedupList->tail =   dedupPtr;
            dedupList->counter  ++;
            pthread_cond_signal(&dedupList->rCond);
            if(dedupList->counter == OTHER_LIST_LEN)
                pthread_cond_wait(&dedupList->wCond, &dedupList->mutex);
            pthread_mutex_unlock(&dedupList->mutex);

            decodeTmp   =   decodePtr->next;
            free(decodePtr->in_d);
            free(decodePtr);
            decodePtr   =   decodeTmp;
        }
        else 
        {
            decodeTail->next    =   decodePtr;
            decodeTail  =   decodePtr;
            decodePtr   =   decodePtr->next;
            decodeTail->next    =   NULL;
        }
    }

    pthread_mutex_lock(&dedupList->mutex);
    dedupList->ending   =   1;
    pthread_mutex_unlock(&dedupList->mutex);
    pthread_cond_signal(&dedupList->rCond);
}

void* de_encode_and_write_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    List        dedupList   =   (List)arg[0];
    char        *outPath    =   (char*)arg[1];
    #ifdef  CHECK_DECOMPRESS
    char        *oriPath    =   (char*)arg[2];
    char        oriFilePath[MAX_PATH_LEN];
    #endif
    de_dedupPtr dedupPtr, dedupTmp;
    char        outFilePath[MAX_PATH_LEN];
    uint64_t    *reSize     =   (uint64_t*)g_malloc0(sizeof(uint64_t));

    while(1)
    {
        pthread_mutex_lock(&dedupList->mutex);
        if(dedupList->counter == 0)
        {
            if(dedupList->ending)  break;
            else 
            {
                pthread_cond_wait(&dedupList->rCond, &dedupList->mutex);
                if(dedupList->counter == 0)    break;
            }
        }
        dedupPtr   =   dedupList->head;
        dedupList->head    =   NULL;
        dedupList->tail    =   NULL;
        dedupList->counter =   0;
        pthread_mutex_unlock(&dedupList->mutex);
        pthread_cond_signal(&dedupList->wCond);

        while(dedupPtr)
        {
            PUT_3_STRS_TOGETHER(outFilePath,outPath,"/",dedupPtr->name);
            #ifdef  CHECK_DECOMPRESS
            PUT_3_STRS_TOGETHER(oriFilePath,oriPath,"/",dedupPtr->name);
            #endif
            *reSize +=  de_encode_a_single_img(outFilePath,dedupPtr->coe,\
                dedupPtr->content->header,dedupPtr->content->headerSize,\
                dedupPtr->ffxx,dedupPtr->xx
                #ifdef CHECK_DECOMPRESS
                    , oriFilePath
                #endif
                );

            free(dedupPtr->coe[0]);
            free(dedupPtr->coe[1]);
            free(dedupPtr->coe[2]);
            free(dedupPtr->coe);
            free(dedupPtr->oriPtr);
            dedupTmp    =   dedupPtr->next;
            free(dedupPtr);
            dedupPtr    =   dedupTmp;
        }
    }

    return (void*)reSize;
}

void free_hashVal(gpointer p)
{
    jpeg_coe_ptr    jcp =   (jpeg_coe_ptr)p;
    free(jcp->data);
    #ifdef HEADER_DELTA
    free(jcp->header);
    #endif
    free(jcp);
}

void free_hashKey(gpointer p)
{
    free(p);
}

uint64_t idedup_decompress(char *inFolder, char *outFolder
    #ifdef  CHECK_DECOMPRESS
        , char *oriFolder
    #endif
)
{
    GHashTable  *coeTable   =   g_hash_table_new_full(g_str_hash,g_str_equal,free_hashKey, free_hashVal);
    List        tableList   =   (List)malloc(sizeof(ListNode));
    tableList->head =   coeTable;
    pthread_mutex_init(&tableList->mutex,NULL);
    uint64_t    avaiSize    =   0;

    List        readList    =   (List)malloc(sizeof(ListNode));
    List        decdList    =   (List)malloc(sizeof(ListNode));
    INIT_LIST(readList);
    INIT_LIST(decdList);
    pthread_t   read_t_id, decd_t_id;
    void        *read_arg[] =   {inFolder, readList, tableList};
    void        *decd_arg[] =   {readList, decdList};
    void        *rawSize;
    pthread_create(&read_t_id, NULL, de_read_thread, (void*)read_arg);
    pthread_create(&decd_t_id, NULL, de_decode_thread, (void*)decd_arg);
    pthread_join(read_t_id, (void**)(&rawSize));
    pthread_join(decd_t_id, NULL);
    avaiSize    +=  *((uint64_t*)rawSize);
    free(rawSize);

    List        deupList    =   (List)malloc(sizeof(ListNode));
    INIT_LIST(deupList);
    pthread_t   deup_t_id, enco_t_id;
    void        *deup_arg[] =   {decdList, deupList, tableList};
    void        *enco_arg[] =   {deupList, outFolder
                                    #ifdef  CHECK_DECOMPRESS
                                        , oriFolder
                                    #endif
                                };
    void        *restSize;
    pthread_create(&deup_t_id, NULL, de_dedup_thread, (void*)deup_arg);
    pthread_create(&enco_t_id, NULL, de_encode_and_write_thread, (void*)enco_arg);
    pthread_join(deup_t_id, NULL);
    pthread_join(enco_t_id, (void**)(&restSize));
    avaiSize    +=  *((uint64_t*)restSize);
    free(restSize);

    DESTROY_LIST(readList);
    DESTROY_LIST(decdList);
    DESTROY_LIST(deupList);

    pthread_mutex_destroy(&tableList->mutex);
    g_hash_table_destroy(coeTable);
    free(tableList);

    return avaiSize;
}