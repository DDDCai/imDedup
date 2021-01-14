/*
 * @Author: Cai Deng
 * @Date: 2021-01-14 14:38:26
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-14 14:41:39
 * @Description: 
 */
#include "rejpeg.h"

typedef struct 
{
    short       *value;
    u_int8_t    *length;
    u_int32_t   valLen, lenLen;

}   rleResult;

#ifdef ZIGZAG
char zz_sequence[64] = 
{
    0 ,1 ,5 ,6 ,14,15,27,28,
    2 ,4 ,7 ,13,16,26,29,42,
    3 ,8 ,12,17,25,30,41,43,
    9 ,11,18,24,31,40,44,53,
    10,19,23,32,39,45,52,54,
    20,22,33,38,46,51,55,60,
    21,34,37,47,50,56,59,61,
    35,36,48,49,57,58,62,63,
};

static short* zigzag(short *ptr)
{
    short   *tmp =  (short*)malloc(sizeof(short)<<6);
    int     i;

    for(i=0;i<64;i++)
    {
        tmp[zz_sequence[i]] =   ptr[i];
    }

    return tmp;
}
#endif  // #ifdef ZIGZAG

// static rleResult rleEncode(GPtrArray *dataPtr
static rleResult rleEncode(GArray *dataPtr
    #ifdef JPEG_SEPA_COMP
    , uint32_t from, uint32_t componentLen
    #endif
)
{
    u_int32_t   valLen  = 0, lenLen = 0;
    #ifdef JPEG_SEPA_COMP
    u_int8_t    *length = (u_int8_t*)malloc(componentLen << 6);
    short       *value  = (short*)malloc(sizeof(short)*componentLen << 6);
    #else
    u_int8_t    *length = (u_int8_t*)malloc(dataPtr->len << 6);
    short       *value  = (short*)malloc(sizeof(short)*dataPtr->len << 6);
    #endif
    u_int32_t   zeroCounter  = 0;
    int         i, j, h;
    short       *ptr;
    short       lastDC  = 0, thisDC;
    rleResult   result;

    #ifndef JPEG_SEPA_COMP
    for(i=0;i<dataPtr->len;i++)
    #else
    for(i=0;i<componentLen;i++)
    #endif
    {
        #ifndef JPEG_SEPA_COMP
        // ptr = (short*)g_ptr_array_index(dataPtr,i);
        ptr = (short*)g_array_index(dataPtr, uint8_t*, i);
        #else
        // ptr = (short*)g_ptr_array_index(dataPtr,i+from);
        ptr = (short*)g_array_index(dataPtr,uint8_t*,i+from);
        #endif
        #ifdef  ZIGZAG
        ptr = zigzag(ptr);
        #endif
        thisDC = ptr[0];
        ptr[0] = thisDC - lastDC;
        lastDC = thisDC;

        for(h=63;h>=0;h--)
            if(ptr[h]) break;

        for(j=0;j<=h;j++)
        {
            if(ptr[j])
            {
                value[valLen++] = ptr[j];
                length[lenLen++] = zeroCounter;
                zeroCounter = 0;
            }
            else 
            {
                if(zeroCounter == 15)
                {
                    value[valLen++] = 0;
                    length[lenLen++] = 15;
                    zeroCounter = 0;
                }
                else 
                    zeroCounter ++;
            }
        }
        if(h<63)
        {
            value[valLen++] = 0;
            length[lenLen++] = 0;
        }
        
        #ifdef  ZIGZAG
        free(ptr);
        #else
        ptr[0]  =   lastDC;
        #endif
    }

    result.length = length;
    result.value  = value;
    result.lenLen = lenLen;
    result.valLen = valLen;

    return result;
}

static void vliEncode(rleResult *rle, uint8_t *buf)
{
    u_int8_t    *lenPtr = rle->length;
    short       *valPtr = rle->value;
    u_int16_t   *data   = (u_int16_t*)buf;
    short       number;
    u_int16_t   _number_;
    u_int32_t   size    = rle->valLen;
    u_int8_t    bitLength; 
    u_int16_t   bits;
    int         dataPos = 0, emptyBits = 16, i;

    for(i=0;i<size;i++)
    {
        number   = valPtr[i];
        _number_ = abs(number);
        if     (_number_==0   )   bitLength = 0;
        else if(_number_==1   )   bitLength = 1;
        else if(_number_<=3   )   bitLength = 2;
        else if(_number_<=7   )   bitLength = 3;
        else if(_number_<=15  )   bitLength = 4;
        else if(_number_<=31  )   bitLength = 5;
        else if(_number_<=63  )   bitLength = 6;
        else if(_number_<=127 )   bitLength = 7;
        else if(_number_<=255 )   bitLength = 8;
        else if(_number_<=511 )   bitLength = 9;
        else if(_number_<=1023)   bitLength = 10;
        else if(_number_<=2047)   bitLength = 11;
        else if(_number_<=4095)   bitLength = 12;

        lenPtr[i] = (lenPtr[i] << 4) | bitLength;

        if(number)
        {
            if(number > 0)  bits = number;
            else bits = (-number) ^ (0xffff >> (16 - bitLength));

            if(emptyBits >= bitLength)
            {
                emptyBits -= bitLength;
                data[dataPos] |= bits << emptyBits;
            }
            else 
            {
                bitLength -= emptyBits;
                data[dataPos] |= bits >> bitLength;
                emptyBits = 16 - bitLength;
                data[++dataPos] |= bits << emptyBits;
            }
        }
    }

    free(rle->value);
    rle->value = data;
    if(size)
        rle->valLen = dataPos + 1;
}

/**
 * [4 bytes] (the size of value data : n);
 * [n bytes] (value data);
 * [1 byte ] (if length data has been compressed);
 * [4 bytes] (the size of length data : m);
 * [m bytes] (length data).
 * */
// static int recompressed_by_myjpeg(GPtrArray *dataPtr, uint8_t *buf, uint32_t bufSize
static int recompressed_by_myjpeg(GArray *dataPtr, uint8_t *buf, uint32_t bufSize
    #ifdef JPEG_SEPA_COMP
    , uint32_t from, uint32_t compoentLen
    #endif
)
{
    uint32_t size   =   sizeof(uint32_t);
    uint8_t *ptr    =   buf + size;
    rleResult rle   =   rleEncode(dataPtr
        #ifdef JPEG_SEPA_COMP
        ,from,compoentLen
        #endif
        );
    vliEncode(&rle,ptr);

    uint32_t valSize=   rle.valLen*sizeof(uint16_t);
    memcpy(buf, &valSize, sizeof(valSize));
    ptr += valSize;
    size+= valSize;

    size    +=  (sizeof(uint8_t) + sizeof(uint32_t));
    uint8_t *pre = ptr;
    ptr += (sizeof(uint8_t) + sizeof(uint32_t));
    uint32_t lenLen = entropy_compress(rle.length, rle.lenLen, ptr, bufSize - size);
    if(lenLen > 0)
        *pre++ = 1;
    else 
    {
        if(rle.lenLen > (bufSize - size))
        {
            free(rle.length);
            return -1;
        }
        *pre++ = 0;
        memcpy(ptr, rle.length, rle.lenLen);
        lenLen = rle.lenLen;
    }
    ptr  += lenLen;
    size += lenLen;
    memcpy(pre, &lenLen, sizeof(lenLen));

    free(rle.length);

    return size;
}

rejpegResPtr rejpeg_a_single_img(dedupResPtr dedupPtr)
{
    uint32_t    bfSize  =   dedupPtr->insert_p->len << 7;
    uint8_t     *buf    =   NULL;
    int         size    =   0;
    rejpegResPtr    rejpegPtr   =   (rejpegResPtr)malloc(sizeof(rejpegResNode));

    for( ; bfSize>0; bfSize <<= 1)
    {
        #ifndef JPEG_SEPA_COMP
        buf     =   (uint8_t*)g_malloc0(bfSize);
        size    =   recompressed_by_myjpeg(dedupPtr->insert_p, buf, bfSize);
        if(size < 0)
            free(buf);
        else 
            break;
        #else
        size = 0;
        int sizeTmp;
        buf     =   (uint8_t*)g_malloc0(bfSize);
        sizeTmp =   recompressed_by_myjpeg(dedupPtr->insert_p, buf, bfSize, 0, dedupPtr->p_counter[0]);
        if(sizeTmp < 0)
        {
            free(buf);
            continue;
        }
        size += sizeTmp;
        sizeTmp =   recompressed_by_myjpeg(dedupPtr->insert_p, buf+size, bfSize-size, dedupPtr->p_counter[0], dedupPtr->p_counter[1]);
        if(sizeTmp < 0)
        {
            free(buf);
            continue;
        }
        size += sizeTmp;
        sizeTmp =   recompressed_by_myjpeg(dedupPtr->insert_p, buf+size, bfSize-size, dedupPtr->p_counter[0]+dedupPtr->p_counter[1], dedupPtr->p_counter[2]);
        if(sizeTmp < 0)
        {
            free(buf);
            continue;
        }
        size += sizeTmp;
        break;
        #endif
    }

    rejpegPtr->dedupRes     =   dedupPtr;
    rejpegPtr->rejpegRes    =   buf;
    rejpegPtr->rejpegSize   =   size;

    return  rejpegPtr;
}

/*-------------------------------------------------------------------------*/

static void vliDecode(rleResult *rle)
{
    u_int32_t   sum        = rle->lenLen;
    u_int8_t    lengthTmp, length;
    u_int8_t    *lengthPtr = rle->length;
    u_int16_t   *dataPtr   = rle->value;
    short       *buf       = (short*)malloc(sizeof(short)*sum);
    int         fullBits   = 16;
    u_int16_t   bits,      dataTmp;
    int         i;

    dataTmp = *dataPtr++;
    for(i=0;i<sum;i++)
    {
        lengthTmp = lengthPtr[i] & 0x0f;
        length = lengthTmp;
        if(lengthTmp == 0)
            buf[i] = 0;
        else 
        {
            bits = 0;
            if(fullBits < lengthTmp)
            {
                bits = dataTmp >> (16 - fullBits);
                lengthTmp -= fullBits;
                bits <<= lengthTmp;
                fullBits = 16;
                dataTmp = *dataPtr++;
            }
            bits |= dataTmp >> (16 - lengthTmp);
            fullBits -= lengthTmp;
            dataTmp <<= lengthTmp;

            if((bits >> (length - 1)))
                buf[i] = bits;
            else 
                buf[i] = -(bits ^ (0xffff >> (16 - length)));
        }
    }

    rle->value  = buf;
    rle->valLen = rle->lenLen;
}

#ifdef  ZIGZAG
static void de_zigzag(short *ori, short *dst)
{
    for(int i=0;i<64;i++)
        dst[i] =   ori[zz_sequence[i]];
}
#endif  // #ifdef  ZIGZAG

static void rleDecode(rleResult *rle)
{
    short       *dctTmp     = (short*)g_malloc0(sizeof(short)*rle->lenLen<<6);
    short       *dctPtr     = dctTmp;
    u_int8_t    *lengthPtr  = rle->length;
    short       *dataPtr    = rle->value;
    u_int32_t   lenLen      = rle->lenLen;
    u_int32_t   blockLeft   = 64;
    u_int8_t    lengthTmp;
    int         i;
    short       lastDC      = 0;
    #ifdef  ZIGZAG
    short       *zigzagTmp  = (short*)malloc(sizeof(short)*rle->lenLen<<6);
    short       *zigzagPtr  = zigzagTmp;
    #endif
int counter = 0;
    for(i=0;i<lenLen;i++)
    {
        if(lengthTmp = (lengthPtr[i] >> 4))
        {
            dctPtr += lengthTmp;
            *dctPtr++ = dataPtr[i];
            blockLeft -= (lengthTmp + 1);
        }
        else 
        {
            if(dataPtr[i])
            {
                *dctPtr++ = dataPtr[i];
                blockLeft --;
            }
            else 
            {
                dctPtr += blockLeft;
                blockLeft = 0;
            }
        }
        if(blockLeft == 0)
        {counter ++;
            blockLeft = 64;
            *(dctPtr - 64) += lastDC;
            lastDC = *(dctPtr - 64);
            #ifdef  ZIGZAG
            de_zigzag(dctPtr-64,zigzagPtr);
            zigzagPtr += 64;
            #endif
        }
    }

    #ifdef  ZIGZAG
    free(dctTmp);
    dctTmp  =   zigzagTmp;
    #endif
    free(rle->value);
    rle->value = dctTmp;
}

short *decode_myjpeg(uint8_t *data)
{
    uint8_t     *ptr    = data;
    uint32_t    valSize = *((uint32_t*)ptr);
    ptr +=  sizeof(uint32_t);
    uint8_t     *val    = ptr;
    ptr +=  valSize;
    uint8_t     ifCmpd  = *ptr++;
    uint32_t    lenSize;
    memcpy(&lenSize, ptr, sizeof(lenSize));
    ptr +=  sizeof(uint32_t);
    uint8_t     *len    = ptr;

    uint8_t     *length = (uint8_t*)malloc(lenSize<<10);
    rleResult   rle;
    if(ifCmpd)
    {
        rle.lenLen = FSE_decompress(length, lenSize<<10, len, lenSize);
        if(rle.lenLen <= 0)
            printf("fail to decompress in *decode_myjpeg*.\n");
        rle.length = length;
    }
    else 
    {
        rle.lenLen = lenSize;
        rle.length = len;
    }
    rle.value = (short*)val;

    vliDecode(&rle);
    rleDecode(&rle);

    free(length);

    return rle.value;
}

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

uint32_t de_encode_a_single_img(char *outPath, jvirt_barray_ptr *coe,
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