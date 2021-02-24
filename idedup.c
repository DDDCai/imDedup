/*
 * @Author: Cai Deng
 * @Date: 2020-11-09 14:24:32
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-02-24 14:06:20
 * @Description: 
 */
#include "idedup.h"
#include "2df.h"
#include "idelta.h"
#include "jpeg.h"
#include "rejpeg.h"
#include "buffer.h"

extern uint8_t chunking_mode;

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

static void* name_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    char        *folderPath =   (char*)arg[0];
    List        nameList    =   (List)arg[1];
    DIR         *f_dir, *s_dir;
    struct      dirent      *f_entry, *s_entry;
    char        inPath[MAX_PATH_LEN];
    nameDataPtr namePtr;
    
    if(!(f_dir = opendir(folderPath)))
    {
        printf("fail to open folder %s\n", folderPath);
        exit(EXIT_FAILURE);
    }
    while(f_entry = readdir(f_dir))
    {
        if(!strcmp(f_entry->d_name,".") || !strcmp(f_entry->d_name,".."))
            continue;
        PUT_3_STRS_TOGETHER(inPath, folderPath, "/", f_entry->d_name);

        s_dir   =   opendir(inPath);
        while(s_entry = readdir(s_dir))
        {
            if(!strcmp(s_entry->d_name, ".") || !strcmp(s_entry->d_name, ".."))
                continue;
            namePtr =   (nameDataPtr)malloc(sizeof(nameDataNode));
            namePtr->next   =   NULL;
            strcpy(namePtr->second_dir, f_entry->d_name);
            strcpy(namePtr->file_name, s_entry->d_name);
            namePtr->mem_size   =   sizeof(nameDataNode);
            namePtr->end_of_dir =   0;

            pthread_mutex_lock(&nameList->mutex);
            while(nameList->size < namePtr->mem_size)
                pthread_cond_wait(&nameList->wCond, &nameList->mutex);
            if(nameList->head)
                ((nameDataPtr)nameList->tail)->next =   namePtr;
            else 
                nameList->head  =   namePtr;
            nameList->tail  =   namePtr;
            nameList->counter   ++;
            nameList->size  -=  namePtr->mem_size;
            pthread_cond_signal(&nameList->rCond);
            pthread_mutex_unlock(&nameList->mutex);
        }
        namePtr->end_of_dir =   1;  //  default : no folder is empty!
        closedir(s_dir);
    }
    closedir(f_dir);

    pthread_mutex_lock(&nameList->mutex);
    nameList->ending    =   1;
    pthread_cond_signal(&nameList->rCond);
    pthread_mutex_unlock(&nameList->mutex);
}

static void* read_thread(void *parameter)
{
    void        **arg       =   (void**)parameter;
    List        nameList    =   (List)arg[0];
    List        rawList     =   (List)arg[1];
    char        *folderPath =   (char*)arg[2];
    nameDataPtr namePtr;
    rawDataPtr  rawPtr;
    char        filePath[MAX_PATH_LEN];
    uint8_t     *rawDataBuffer;
    uint64_t    fileSize;
    struct      stat        statbuf;
    FILE        *fp;
    uint64_t    *rawSize    =   (uint64_t*)g_malloc0(sizeof(uint64_t));
    uint8_t     end_of_dir;
    #ifdef PART_TIME
    GTimer      *timer      =   g_timer_new();
    #endif

    while(1)
    {
        pthread_mutex_lock(&nameList->mutex);
        while(nameList->counter == 0)
        {
            if(nameList->ending) goto ESCAPE_LOOP;
            pthread_cond_wait(&nameList->rCond, &nameList->mutex);
        }
        namePtr =   nameList->head;
        nameList->head  =   namePtr->next;
        nameList->counter   --;
        nameList->size  +=  namePtr->mem_size;
        pthread_cond_signal(&nameList->wCond);
        pthread_mutex_unlock(&nameList->mutex);

        if(namePtr)
        {
            #ifdef PART_TIME
            g_timer_start(timer);
            #endif

            end_of_dir  =   namePtr->end_of_dir;
            rawPtr      =   (rawDataPtr)malloc(sizeof(rawDataNode));

            PUT_3_STRS_TOGETHER(filePath, folderPath, "/", namePtr->second_dir);
            rawPtr->dir_name    =   (char*)malloc(strlen(filePath) + 1);
            strcpy(rawPtr->dir_name, filePath);
            PUT_3_STRS_TOGETHER(filePath, filePath, "/", namePtr->file_name);
            stat(filePath, &statbuf);
            fileSize    =   statbuf.st_size;
            rawDataBuffer   =   (uint8_t*)malloc(fileSize);
            fp  =   fopen(filePath, "rb");
            if(fileSize != fread(rawDataBuffer, 1, fileSize, fp))
            {
                printf("fail to read %s\n", filePath);
                free(rawDataBuffer);
                free(rawPtr);
                fclose(fp);
                continue ;
            }
            fclose(fp);

            rawPtr->data    =   rawDataBuffer;
            rawPtr->size    =   fileSize;
            rawPtr->name    =   (char*)malloc(strlen(namePtr->file_name) + 1);
            strcpy(rawPtr->name, namePtr->file_name);
            rawPtr->mem_size=   sizeof(rawDataNode) + fileSize + strlen(namePtr->file_name) + strlen(rawPtr->dir_name) + 2;
            rawPtr->next    =   NULL;

            #ifdef PART_TIME
            read_time   +=  g_timer_elapsed(timer, NULL);
            #endif

            pthread_mutex_lock(&rawList->mutex);
            while(rawList->size < rawPtr->mem_size)
                pthread_cond_wait(&rawList->wCond, &rawList->mutex);
            if(rawList->head)
                ((rawDataPtr)rawList->tail)->next   =   rawPtr;
            else
                rawList->head   =   rawPtr;
            rawList->tail   =   rawPtr;
            rawList->counter    ++;
            rawList->size   -=  rawPtr->mem_size;
            pthread_cond_signal(&rawList->rCond);
            pthread_mutex_unlock(&rawList->mutex);

            *rawSize +=  fileSize;
            free(namePtr);
        }

        if(chunking_mode)
            if(*rawSize >= PATCH_SIZE/READ_THREAD_NUM) goto ESCAPE_LOOP_1;
        else
            if(end_of_dir && (*rawSize >= PATCH_SIZE*3/4))  goto ESCAPE_LOOP_1;
    }

    ESCAPE_LOOP:
    pthread_mutex_unlock(&nameList->mutex);
    ESCAPE_LOOP_1:

    #ifdef PART_TIME
    g_timer_destroy(timer);
    #endif

    pthread_mutex_lock(&rawList->mutex);
    rawList->ending  ++;
    for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        pthread_cond_signal(&rawList->rCond);
    pthread_mutex_unlock(&rawList->mutex);

    return  (void*)rawSize;
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
    rejpegResPtr    rejpegPtr;

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
        rejpegList->head    =   rejpegPtr->next;
        rejpegList->counter --;
        rejpegList->size    +=  rejpegPtr->mem_size;
        for(int i=0; i<MIDDLE_THREAD_NUM; i++)
            pthread_cond_signal(&rejpegList->wCond);
        pthread_mutex_unlock(&rejpegList->mutex);

        if(rejpegPtr)
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
            free(rejpegPtr);

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
    struct rabin_t *h = rabin_init();
    free(h);
    List        nameList;
    INIT_LIST(nameList, NAME_LIST_MAX);
    pthread_t   name_t_id;
    void        *name_arg[] =   {inFolder, nameList};
    pthread_create(&name_t_id, NULL, name_thread, (void*)name_arg);

    #ifdef DEBUG_1
    uint64_t    *result =   (uint64_t*)g_malloc0(sizeof(uint64_t)*11);
    #else
    uint64_t    *result =   (uint64_t*)g_malloc0(sizeof(uint64_t)*3);
    #endif

    uint32_t    patch_id    =   0;
    char        outPath[MAX_PATH_LEN];

    while(1)
    {
        pthread_mutex_lock(&nameList->mutex);
        while(nameList->counter == 0)
        {
            if(nameList->ending) goto ESCAPE_LOOP;
            pthread_cond_wait(&nameList->rCond, &nameList->mutex);
        }
        pthread_mutex_unlock(&nameList->mutex);

        sprintf(outPath, "%s/%u", outFolder, patch_id);
        if(access(outPath, 0) < 0)
            mkdir(outPath, 0755);

        Buffer      decodeBuffer;
        decodeBuffer.head   =   NULL;
        decodeBuffer.tail   =   NULL;
        decodeBuffer.size   =   DECODE_BUFFER_SIZE;
        pthread_mutex_init(&decodeBuffer.mutex, NULL);
        
        List        rawList, decList[MIDDLE_THREAD_NUM], detList[MIDDLE_THREAD_NUM], dupList[MIDDLE_THREAD_NUM], rejList;
        INIT_LIST(rawList, READ_LIST_MAX);
        for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        {
            INIT_LIST(decList[i], DECD_LIST_MAX/MIDDLE_THREAD_NUM);
            INIT_LIST(detList[i], DECT_LIST_MAX/MIDDLE_THREAD_NUM);
            INIT_LIST(dupList[i], DEUP_LIST_MAX/MIDDLE_THREAD_NUM);
        }
        INIT_LIST(rejList, REJG_LIST_MAX);

        GHashTable      *featureT[SF_NUM];
        pthread_mutex_t ftMutex[SF_NUM];
        for(int i=0; i<SF_NUM; i++)
        {
            featureT[i] =   g_hash_table_new_full(g_int64_hash, g_int64_equal, NULL, free_ht_val);
            pthread_mutex_init(&ftMutex[i], NULL);
        }

        pthread_t   read_t_id[READ_THREAD_NUM], decd_t_id[MIDDLE_THREAD_NUM], detc_t_id[MIDDLE_THREAD_NUM], 
                    dedup_t_id[MIDDLE_THREAD_NUM], rejpg_t_id[MIDDLE_THREAD_NUM], writ_t_id[WRITE_THREAD_NUM];

        void        *read_arg[] = {nameList, rawList, inFolder};
        void        **decd_arg[MIDDLE_THREAD_NUM], **detc_arg[MIDDLE_THREAD_NUM], **dedu_arg[MIDDLE_THREAD_NUM], **reje_arg[MIDDLE_THREAD_NUM];
        for(int i=0; i<MIDDLE_THREAD_NUM; i++)
        {
            decd_arg[i] = (void**)malloc(sizeof(void*)*3);
            detc_arg[i] = (void**)malloc(sizeof(void*)*6);
            dedu_arg[i] = (void**)malloc(sizeof(void*)*2);
            reje_arg[i] = (void**)malloc(sizeof(void*)*3);
            decd_arg[i][0] = outPath;
            decd_arg[i][1] = rawList;
            decd_arg[i][2] = decList[i];
            detc_arg[i][0] = decList[i];
            detc_arg[i][1] = detList[i];
            detc_arg[i][2] = outPath;
            detc_arg[i][3] = featureT;
            detc_arg[i][4] = ftMutex;
            detc_arg[i][5] = &decodeBuffer;
            dedu_arg[i][0] = detList[i];
            dedu_arg[i][1] = dupList[i];
            reje_arg[i][0] = dupList[i];
            reje_arg[i][1] = rejList;
            reje_arg[i][2] = outPath;
        }
        void        *writ_arg[] = {rejList, outPath};

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
        {
            free(undecdSize[i]);
            free(unhandledSize[i]);
        }
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

        patch_id    ++;
    }

    ESCAPE_LOOP:
    pthread_mutex_unlock(&nameList->mutex);

    pthread_join(name_t_id, NULL);
    DESTROY_LIST(nameList);

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
            // if(readList->counter == READ_LIST_LEN)
            //     pthread_cond_wait(&readList->wCond, &readList->mutex);
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
    // INIT_LIST(readList);
    // INIT_LIST(decdList);
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
    // INIT_LIST(deupList);
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