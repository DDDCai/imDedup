/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 12:50:48
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-19 19:41:41
 * @Description: 
 */
#include "jpeg.h"
#include "idelta.h"

jpeg_coe_ptr get_base_coe_mem(uint8_t *data, uint32_t size)
{
    if(size<0 || data[0]!=0xff || data[1]!=0xd8 || data[size-2]!=0xff || data[size-1]!=0xd9)
        return NULL;

    struct jpeg_decompress_struct dinfo;
    struct jpeg_error_mgr jerr;
    
    dinfo.err = jpeg_std_error(&jerr);
    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo,data,size);
    if(jpeg_read_header(&dinfo,TRUE)!=1 || dinfo.num_components!=3
        #ifdef  NO_PROGRESSIVE
        || dinfo.progressive_mode
        #endif
    )
    {
        jpeg_destroy_decompress(&dinfo);
        return NULL;
    }

    jpeg_coe_ptr coe_result = (jpeg_coe_ptr)malloc(sizeof(jpeg_coe));
    coe_result->header      = data;
    coe_result->headerSize  = size - dinfo.src->bytes_in_buffer;
    jvirt_barray_ptr   *coe = jpeg_read_coefficients(&dinfo);

    coe_result->imgSize[0] = dinfo.comp_info[0].width_in_blocks;
    coe_result->imgSize[1] = dinfo.comp_info[0].height_in_blocks;
    coe_result->imgSize[2] = dinfo.comp_info[1].width_in_blocks;
    coe_result->imgSize[3] = dinfo.comp_info[1].height_in_blocks;
    coe_result->imgSize[4] = dinfo.comp_info[2].width_in_blocks;
    coe_result->imgSize[5] = dinfo.comp_info[2].height_in_blocks;

    coe_result->data    =   (uint8_t*)malloc(sizeof(JBLOCK)*
                            (coe_result->imgSize[0]*coe_result->imgSize[1]+
                            coe_result->imgSize[2]*coe_result->imgSize[3]*2));
    uint8_t     *sbrow  =   coe_result->data;
    JBLOCKROW   jbrow   =   NULL;
    JBLOCKARRAY jbarray;
    int i, j;

    for(int k=0; k<3; k++)
    {
        jbarray = coe[k]->mem_buffer;
        for(i=0; i<dinfo.comp_info[k].height_in_blocks; i++)
        {
            jbrow = jbarray[i];
            for(j=0; j<dinfo.comp_info[k].width_in_blocks; j++,sbrow+=sizeof(JBLOCK))
                memcpy(sbrow,jbrow[j],sizeof(JBLOCK));
        }
    }

    jpeg_finish_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);

    return coe_result;
}

/* to be freed : data, target_ptr, jpeg_coe_ptr, jpe_coe_ptr->j_decompress_ptr
 */
static target_ptr get_target_coe_mem(uint8_t *data, uint32_t size)
{
    jpeg_coe_ptr coe = get_base_coe_mem(data,size);
    if(!coe) 
        return NULL;

    target_ptr tar_result = (target_ptr)malloc(sizeof(target_struct));
    tar_result->coe = coe;

    #ifdef  END_WITH_FFXX
    if(data[size-4]==0xff)
    {
        tar_result->ffxx = 1;
        tar_result->xx = data[size-3];
    }
    else 
        tar_result->ffxx = 0;
    #endif

    return tar_result;
}

static decodedDataPtr decode_a_single_img(rawDataPtr rawPtr)
{
    target_ptr targetInfo = get_target_coe_mem(rawPtr->data, rawPtr->size);
    if(!targetInfo)
        return NULL;
    decodedDataPtr decdPtr  = (decodedDataPtr)malloc(sizeof(decodedDataNode));
    decdPtr->rawData        = rawPtr;
    decdPtr->targetInfo     = targetInfo;
    decdPtr->mem_size       = rawPtr->mem_size + sizeof(decodedDataNode) + sizeof(target_struct)
                            + sizeof(jpeg_coe) + (targetInfo->coe->imgSize[0]*targetInfo->coe->imgSize[1]
                            + targetInfo->coe->imgSize[2]*targetInfo->coe->imgSize[3]
                            + targetInfo->coe->imgSize[4]*targetInfo->coe->imgSize[5]) * sizeof(JBLOCK);

    return decdPtr;
}

void* decode_thread(void *parameter)
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
            if(rawList->ending) goto ESCAPE_LOOP;
            pthread_cond_wait(&rawList->rCond, &rawList->mutex);
        }
        rawPtr  =   rawList->head;
        rawList->head   =   rawPtr->next;
        rawList->size   +=  rawPtr->mem_size;
        rawList->counter    --;
        pthread_cond_signal(&rawList->wCond);
        pthread_mutex_unlock(&rawList->mutex);

        if(rawPtr)
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
                while(decList->size < decPtr->mem_size)
                    pthread_cond_wait(&decList->wCond, &decList->mutex);
                if(decList->counter)
                    ((decodedDataPtr)decList->tail)->next   =   decPtr;
                else
                    decList->head   =   decPtr;
                decList->tail   =   decPtr;
                decList->counter    ++;
                decList->size   -=  decPtr->mem_size;
                pthread_cond_signal(&decList->rCond);
                pthread_mutex_unlock(&decList->mutex);
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
    decList->ending = 1;
    pthread_cond_signal(&decList->rCond);
    pthread_mutex_unlock(&decList->mutex);

    return (void*)undecSize;
}

/*-------------------------------------------------------------------*/

static void check_jpeg_header(u_int8_t *data, u_int32_t *size, u_int8_t target)
{
    int i   =   2;
    while(1)
    {
        while(data[i++]!=0xff) ;
        if(data[i] == target) break;
        else if(data[i]>=0xc0 && data[i]<0xfe)
            i += (((data[i+1]<<8) | data[i+2]) + 1);
    }
    i   ++;
    *size   =   i + ((data[i]<<8) | data[i+1]);
}

static uint32_t de_encode_a_single_img(char *outPath, jvirt_barray_ptr *coe,
    uint8_t *header, uint32_t hedLen, uint8_t ffxx, uint8_t xx
    #ifdef  CHECK_DECOMPRESS
        , char *oriFilePath
    #endif
)
{
    struct      jpeg_decompress_struct   dinfo;
    struct      jpeg_compress_struct     cinfo;
    struct      jpeg_error_mgr           derr,cerr;
    FILE        *fp;
    uint8_t     *buffer,    *data;
    unsigned long bufSize;
    unsigned int  headerSize, i;
    #ifdef  CHECK_DECOMPRESS
    struct      stat        statbuf;
    unsigned int  oriSize,    realSize;
    uint8_t     *oriBuffer;
    FILE        *oriFp;
    #endif

    /*  read information from the original header. */
    dinfo.err   =   jpeg_std_error(&derr);
    jpeg_create_decompress(&dinfo);
    jpeg_mem_src(&dinfo,header,hedLen);
    jpeg_read_header(&dinfo,FALSE);
    jpeg_start_decompress(&dinfo);
    /*  encode the image into buffer. */
    bufSize     =   dinfo.comp_info[0].width_in_blocks*dinfo.comp_info[0].height_in_blocks*3<<6;
    buffer      =   (uint8_t*)malloc(bufSize);
    cinfo.err   =   jpeg_std_error(&cerr);
    jpeg_create_compress(&cinfo);
    jpeg_mem_dest(&cinfo,&buffer,&bufSize);
    jpeg_copy_critical_parameters(&dinfo,&cinfo);
    /*  copy the huffman tables. */
    for(i=0;i<4;i++)
    {
        if(dinfo.dc_huff_tbl_ptrs[i])
        {
            if(cinfo.dc_huff_tbl_ptrs[i] == NULL)
                cinfo.dc_huff_tbl_ptrs[i] = jpeg_alloc_huff_table((j_common_ptr)(&cinfo));
            memcpy(cinfo.dc_huff_tbl_ptrs[i],dinfo.dc_huff_tbl_ptrs[i],sizeof(JHUFF_TBL));
        }
        if(dinfo.ac_huff_tbl_ptrs[i])
        {
            if(cinfo.ac_huff_tbl_ptrs[i] == NULL)
                cinfo.ac_huff_tbl_ptrs[i] = jpeg_alloc_huff_table((j_common_ptr)(&cinfo));
            memcpy(cinfo.ac_huff_tbl_ptrs[i],dinfo.ac_huff_tbl_ptrs[i],sizeof(JHUFF_TBL));
        }
    }
    /*  set the restart interval.*/
    cinfo.restart_interval  =   dinfo.restart_interval;
    /*  write the coefficients.*/
    jpeg_write_coefficients(&cinfo,coe);
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    /*  finish the decompression. */
    jpeg_abort_decompress(&dinfo);
    jpeg_destroy_decompress(&dinfo);
    /*  replace the header. */
    check_jpeg_header(buffer,&headerSize,0xda);
    data    =   buffer + headerSize;
    #ifdef  END_WITH_FFXX
    if(ffxx)
    {
        if(!(buffer[bufSize-4]==0xff && buffer[bufSize-3]==xx))
        {
            buffer[bufSize-2]   =   0xff;
            buffer[bufSize-1]   =   xx;
            buffer[bufSize++]   =   0xff;
            buffer[bufSize++]   =   0xd9;
        }
    }
    #endif

    #ifdef  CHECK_DECOMPRESS
    realSize=   bufSize - headerSize + hedLen;
    /*  read the original image file. */
    stat(oriFilePath,&statbuf);
    oriSize =   statbuf.st_size;
    oriFp   =   fopen(oriFilePath,"rb");
    oriBuffer   =   (uint8_t*)malloc(oriSize);
    if(oriSize != fread(oriBuffer,1,oriSize,oriFp))
        printf("Fail to read %s\n",oriFilePath);
    fclose(oriFp);
    /*  compare the restored image with original image byte by byte. */
    if(oriSize != realSize || memcmp(oriBuffer,header,hedLen) || memcmp(oriBuffer+hedLen,data,bufSize-headerSize))
        printf("%s\n++++++++++++++\n",oriFilePath);
    free(oriBuffer);
    #endif

    #ifndef  DO_NOT_WRITE
    fp  =   fopen(outPath,"wb");
    fwrite(header,1,hedLen,fp);
    fwrite(data,1,bufSize - headerSize,fp);
    fclose(fp);
    #endif

    free(buffer);

    return (bufSize-headerSize+hedLen);
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