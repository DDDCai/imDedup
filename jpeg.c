/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 12:50:48
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-14 14:41:18
 * @Description: 
 */
#include "jpeg.h"

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

decodedDataPtr decode_a_single_img(rawDataPtr rawPtr)
{
    target_ptr targetInfo = get_target_coe_mem(rawPtr->data, rawPtr->size);
    if(!targetInfo)
        return NULL;
    decodedDataPtr decdPtr  = (decodedDataPtr)malloc(sizeof(decodedDataNode));
    decdPtr->rawData        = rawPtr;
    decdPtr->targetInfo     = targetInfo;

    return decdPtr;
}