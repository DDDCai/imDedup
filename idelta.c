/*
 * @Author: Cai Deng
 * @Date: 2020-11-05 09:12:19
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-14 14:32:26
 * @Description: 
 */
#include "idelta.h"
#include "xdelta/xdelta3.h"

typedef struct digest 
{
    COPY_X x;
    COPY_Y y;
    struct  digest *next;

}   digest_node, *digest_ptr;

typedef struct 
{
    digest_ptr tail, head;
    #ifndef THREAD_OPTI
    COPY_X     lastX;
    COPY_Y     lastY;
    #endif

}   digest_list;


static void free_item(gpointer p)
{
    free(p);
}

static void free_digest_list(gpointer p)
{
    digest_list *list = (digest_list*)p;
    digest_ptr ptr = list->head, tmp;
    while(ptr)
    {
        tmp = ptr;
        ptr = ptr->next;
        free(tmp);
    }
    free(list);
}

#ifndef THREAD_OPTI
// static 
GHashTable **create_block_index(jpeg_coe_ptr base)
{
    GHashTable  **subBlockTab   =   (GHashTable**)malloc(sizeof(GHashTable*)*3);
    int     i, row, column, j;
    uint8_t *block_p;
    uint8_t *ptr = base->data;
    #ifdef USE_RABIN
    struct  rabin_t   *hash;
    #else
    uint64_t *hash;
    #endif

    for(i=0; i<3; i++)
    {
        subBlockTab[i] = g_hash_table_new_full(g_int64_hash,g_int64_equal,free_item,free_digest_list);
        uint32_t width = base->imgSize[i*2];
        uint32_t height= base->imgSize[i*2+1];
        JBLOCKROW jbrow[height];
        for(j=0; j<height; j++, ptr+=sizeof(JBLOCK)*width)
            jbrow[j] = (JBLOCKROW)ptr;
        JBLOCKARRAY jbarray = jbrow;
        for(row=0; row<=height-LBS; row++)
        {
            #ifdef USE_RABIN
            hash = rabin_init();
            #else
            hash = (uint64_t*)g_malloc0(sizeof(uint64_t));
            #endif
            for(column=0; column<LBS-1; column++)
            {
                for(j=0; j<LBS; j++)
                {
                    block_p = (uint8_t*)(jbarray[row+j][column]);
                    #ifdef USE_RABIN
                    rabin_slide_a_block(hash,block_p);
                    #else
                    gear_slide_a_block(hash, block_p);
                    #endif
                }
            }
            for(column=0; column<=width-LBS; column++)
            {
                for(j=0; j<LBS; j++)
                {
                    block_p = (uint8_t*)(jbarray[row+j][column+LBS-1]);
                    #ifdef USE_RABIN
                    rabin_slide_a_block(hash,block_p);
                    #else
                    gear_slide_a_block(hash, block_p);
                    #endif
                }
                digest_ptr value = (digest_ptr)malloc(sizeof(digest_node));
                value->x = column;
                value->y = row;
                value->next = NULL;
                #ifdef USE_RABIN
                digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab[i],&hash->digest);
                #else
                digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab[i],hash);
                #endif
                if(list)
                {
                    // If there are several successive identical blocks, 
                    //  we only need to record the first one, which can
                    //  shrink the comparation times dramatically in iDelta
                    //  compression.
                    if(row==list->lastY && column==list->lastX+1)
                    {
                        free(value);
                    }
                    else 
                    {
                        list->tail->next = value;
                        list->tail = value;
                        list->lastY = (COPY_Y)row;
                    }
                    list->lastX = (COPY_X)column;
                }
                else 
                {
                    uint64_t *key = (uint64_t*)malloc(sizeof(uint64_t));
                    #ifdef USE_RABIN
                    *key = hash->digest;
                    #else
                    *key = *hash;
                    #endif
                    list = (digest_list*)malloc(sizeof(digest_list));
                    list->tail = value;
                    list->head = value;
                    list->lastX = column;
                    list->lastY = row;
                    g_hash_table_insert(subBlockTab[i],key,list);
                }
            }
            free(hash);
        }
    }

    return subBlockTab;
}

static void get_instructions(dedupResPtr dedupPtr, jpeg_coe_ptr base, target_ptr target
    #ifdef THREAD_OPTI
    , GHashTable **subBlockTab
    #endif
)
{
    #ifndef THREAD_OPTI
    GHashTable  **subBlockTab = create_block_index(base);
    #endif

    GArray *cpx  =   g_array_new(FALSE, FALSE, sizeof(COPY_X));
    GArray *cpy  =   g_array_new(FALSE, FALSE, sizeof(COPY_Y));
    GArray *cpl  =   g_array_new(FALSE, FALSE, sizeof(COPY_L));
    GArray *inl  =   g_array_new(FALSE, FALSE, sizeof(INSERT_L));
    GArray *inp  =   g_array_new(FALSE, FALSE, sizeof(uint8_t*));
    // GPtrArray *inp  =   g_ptr_array_new();
    uint8_t     *tar_p = target->coe->data, *base_p = base->data;
    uint32_t    counter[3];
    #ifdef JPEG_SEPA_COMP
    uint32_t    p_counter[3];
    #endif

    for(int i=0; i<3; i++)
    {
        uint32_t  tar_width  = target->coe->imgSize[i*2],
                  tar_height = target->coe->imgSize[i*2+1],
                  base_width = base->imgSize[i*2],
                  base_height= base->imgSize[i*2+1];
        JBLOCKROW tarrow[tar_height], baserow[base_height];
        for(int j=0; j<tar_height; j++, tar_p+=sizeof(JBLOCK)*tar_width)
            tarrow[j] = (JBLOCKROW)tar_p;
        for(int j=0; j<base_height; j++, base_p+=sizeof(JBLOCK)*base_width)
            baserow[j] = (JBLOCKROW)base_p;
        JBLOCKARRAY tar_jbarray = tarrow, base_jbarray = baserow;

        for(COPY_Y row=0; row<=tar_height-LBS; row++)
        {
            COPY_X   column = 0;
            INSERT_L in_len = 0;
            while(column <= tar_width-LBS)
            {
                COPY_X x, xmax;
                COPY_Y y, ymax;
                COPY_L len, lenmax = 0;
                uint8_t *data_ptr = (uint8_t*)(tar_jbarray[row][column]);
                #ifdef USE_RABIN
                struct rabin_t *hash = rabin_init();
                rabin_slide_a_block(hash, data_ptr);
                digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab[i], &(hash->digest));
                #else
                uint64_t *hash = (uint64_t*)g_malloc0(sizeof(uint64_t));
                gear_slide_a_block(hash, data_ptr);
                digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab[i], hash);
                #endif
                free(hash);

                if(list)
                {
                    digest_ptr d_item = list->head;
                    while(d_item)
                    {
                        x = d_item->x;
                        y = d_item->y;
                        len = 0;
                        for(int j=x,k=column; j<=base_width-LBS && k<=tar_width-LBS; j++,k++,len++)
                        {
                            if(memcmp(base_jbarray[y][j], tar_jbarray[row][k], WINSIZE))
                                break;
                        }
                        if(len > lenmax)
                        {
                            lenmax = len;
                            xmax = x;
                            ymax = y;
                        }
                        //  The same position is most likely to be the best 
                        //  matching position.
                        // if(x==column && y==row && lenmax>0)
                        //     break;
                        d_item = d_item->next;
                    }
                    if(lenmax)
                    {
                        g_array_append_val(cpx,xmax);
                        g_array_append_val(cpy,ymax);
                        g_array_append_val(cpl,lenmax);
                        g_array_append_val(inl,in_len);
                        in_len = 0;
                        column += lenmax;
                    }
                    else 
                    {
                        // g_ptr_array_add(inp, data_ptr);
                        g_array_append_val(inp,data_ptr);
                        in_len ++;
                        column ++;
                    }
                }
                else 
                {
                    // g_ptr_array_add(inp, data_ptr);
                    g_array_append_val(inp, data_ptr);
                    in_len ++;
                    column ++;
                }
            }
            if(in_len)
                g_array_append_val(inl,in_len);
        }
        counter[i] = inl->len;
        #ifdef JPEG_SEPA_COMP
        p_counter[i] = inp->len;
        #endif
    }

    dedupPtr->y_counter = counter[0];
    dedupPtr->u_counter = counter[1] - counter[0];
    dedupPtr->v_counter = counter[2] - counter[1];
    #ifdef JPEG_SEPA_COMP
    dedupPtr->p_counter[0] = p_counter[0];
    dedupPtr->p_counter[1] = p_counter[1] - p_counter[0];
    dedupPtr->p_counter[2] = p_counter[2] - p_counter[1];
    #endif

    for(int i=0; i<3; i++)
        g_hash_table_destroy(subBlockTab[i]);
    free(subBlockTab);

    dedupPtr->copy_x    =   cpx;
    dedupPtr->copy_y    =   cpy;
    dedupPtr->copy_l    =   cpl;
    dedupPtr->insert_l  =   inl;
    dedupPtr->insert_p  =   inp;
}
#endif

#ifdef THREAD_OPTI
void *index_sub_thread(void *parameter)
{
    void    **arg   =   (void**)parameter;
    uint64_t    width = (uint64_t)arg[0];
    uint64_t    from = (uint64_t)arg[1];
    uint64_t    to = (uint64_t)arg[2];
    JBLOCKARRAY jbarray = (JBLOCKARRAY)arg[3];
    GHashTable  *subBlockTab    =   (GHashTable*)arg[4];
    pthread_mutex_t *mutex = (pthread_mutex_t*)arg[5];
    int j, row, column;
    uint8_t *block_p;
    #ifdef USE_RABIN
    struct  rabin_t   *hash;
    #else
    uint64_t *hash;
    #endif
    uint64_t    tmp, flag; // begainning of the row?

    for(row=from; row<=to; row++)
    {
        tmp = 0;
        flag = 1;
        #ifdef USE_RABIN
        hash = rabin_init();
        #else
        hash = (uint64_t*)g_malloc0(sizeof(uint64_t));
        #endif
        for(column=0; column<LBS-1; column++)
        {
            for(j=0; j<LBS; j++)
            {
                block_p = (uint8_t*)(jbarray[row+j][column]);
                #ifdef USE_RABIN
                rabin_slide_a_block(hash,block_p);
                #else
                gear_slide_a_block(hash, block_p);
                #endif
            }
        }
        for(column=0; column<=width-LBS; column++)
        {
            for(j=0; j<LBS; j++)
            {
                block_p = (uint8_t*)(jbarray[row+j][column+LBS-1]);
                #ifdef USE_RABIN
                rabin_slide_a_block(hash,block_p);
                #else
                gear_slide_a_block(hash, block_p);
                #endif
            }
            #ifdef USE_RABIN
            if(hash->digest != tmp || flag)
            #else
            if(*hash != tmp)
            #endif
            {
                digest_ptr value = (digest_ptr)malloc(sizeof(digest_node));
                value->x = column;
                value->y = row;
                value->next = NULL;
                pthread_mutex_lock(mutex);
                #ifdef USE_RABIN
                digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab,&hash->digest);
                #else
                digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab,hash);
                #endif
                if(list)
                {
                    list->tail->next = value;
                    list->tail = value;
                }
                else 
                {
                    uint64_t *key = (uint64_t*)malloc(sizeof(uint64_t));
                    #ifdef USE_RABIN
                    *key = hash->digest;
                    #else
                    *key = *hash;
                    #endif
                    list = (digest_list*)malloc(sizeof(digest_list));
                    list->tail = value;
                    list->head = value;
                    g_hash_table_insert(subBlockTab,key,list);
                }
                pthread_mutex_unlock(mutex);
                #ifdef USE_RABIN
                tmp = hash->digest;
                #else
                tmp = *hash;
                #endif
            }
            flag = 0;
        }
        free(hash);
    }
}

void *index_thread(void *parameter)
{
    void     **arg   =   (void**)parameter;
    uint64_t width   =   (uint64_t)arg[0];
    uint64_t height  =   (uint64_t)arg[1];
    uint8_t  *ptr    =   (uint8_t*)arg[2];
    pthread_mutex_t *mutex = (pthread_mutex_t*)arg[3];
    GHashTable *subBlockTab = g_hash_table_new_full(g_int64_hash,g_int64_equal,free_item,free_digest_list);
    JBLOCKROW jbrow[height];
    for(int j=0; j<height; j++, ptr+=sizeof(JBLOCK)*width)
        jbrow[j] = (JBLOCKROW)ptr;
    JBLOCKARRAY jbarray = jbrow;
    pthread_t   pid[INDEX_THREAD_NUM];
    void        **para[INDEX_THREAD_NUM];
    uint64_t    ave_height  =   height/INDEX_THREAD_NUM;

    for(int i=0; i<INDEX_THREAD_NUM; i++)
    {
        para[i] = (void**)malloc(sizeof(void*)*6);
        para[i][0] = (void*)width;
        para[i][1] = (void*)(uint64_t)(i*ave_height);
        para[i][2] = (void*)(uint64_t)((i == INDEX_THREAD_NUM-1)? (height-LBS):((i+1)*ave_height-1));
        para[i][3] = jbarray;
        para[i][4] = subBlockTab;
        para[i][5] = mutex;

        pthread_create(&pid[i], NULL, index_sub_thread, (void*)para[i]);
    }

    for(int i=0; i<INDEX_THREAD_NUM; i++)
    {
        pthread_join(pid[i], NULL);
        free(para[i]);
    }

    return subBlockTab;
}

GHashTable **create_block_index(jpeg_coe_ptr base)
{
    GHashTable  **subBlockTab   =   (GHashTable**)malloc(sizeof(GHashTable*)*3);
    pthread_mutex_t mutex[3];
    uint8_t     *ptr[3];
    uint64_t    width[3], height[3];
    for(int i=0; i<3; i++)
    {
        width[i] = base->imgSize[i*2];
        height[i] = base->imgSize[i*2+1];
    }
    ptr[0] = base->data;
    for(int i=1; i<3; i++)
        ptr[i] = ptr[i-1] + width[i-1]*height[i-1]*sizeof(JBLOCK);

    pthread_t   pid[3];
    void        **arg[3];
    for(int i=0; i<3; i++)
    {
        arg[i]  =   (void**)malloc(sizeof(void*)*4);
        pthread_mutex_init(&mutex[i], NULL);
        arg[i][0] = (void*)width[i];
        arg[i][1] = (void*)height[i];
        arg[i][2] = ptr[i];
        arg[i][3] = &mutex[i];

        pthread_create(&pid[i], NULL, index_thread, (void*)arg[i]);
    }

    for(int i=0; i<3; i++)
    {
        pthread_join(pid[i], (void**)(&subBlockTab[i]));
        free(arg[i]);
        pthread_mutex_destroy(&mutex[i]);
    }

    return subBlockTab;
}

static void* get_instructions_thread(void *parameter)
{
    void        **arg   =   (void**)parameter;
    GArray      *cpx    =   (GArray*)arg[0],
                *cpy    =   (GArray*)arg[1],
                *cpl    =   (GArray*)arg[2],
                *inl    =   (GArray*)arg[3];
    GPtrArray   *inp    =   (GPtrArray*)arg[4];
    JBLOCKARRAY tar_jbarray =   (JBLOCKARRAY)arg[5],
                base_jbarray=   (JBLOCKARRAY)arg[6];
    uint64_t    tar_width   = (uint64_t)arg[7],
                base_width  = (uint64_t)arg[8],
                tar_from    = (uint64_t)arg[9],
                tar_to      = (uint64_t)arg[10];
    GHashTable  *subBlockTab= (GHashTable*)arg[11];

    for(COPY_Y row=tar_from; row<=tar_to; row++)
    {
        COPY_X   column = 0;
        INSERT_L in_len = 0;
        while(column <= tar_width-LBS)
        {
            COPY_X x, xmax;
            COPY_Y y, ymax;
            COPY_L len, lenmax = 0;
            uint8_t *data_ptr = (uint8_t*)(tar_jbarray[row][column]);
            #ifdef USE_RABIN
            struct rabin_t *hash = rabin_init();
            rabin_slide_a_block(hash, data_ptr);
            digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab, &(hash->digest));
            #else
            uint64_t *hash = (uint64_t*)g_malloc0(sizeof(uint64_t));
            gear_slide_a_block(hash, data_ptr);
            digest_list *list = (digest_list*)g_hash_table_lookup(subBlockTab, hash);
            #endif
            free(hash);

            if(list)
            {
                digest_ptr d_item = list->head;
                while(d_item)
                {
                    x = d_item->x;
                    y = d_item->y;
                    len = 0;
                    for(int j=x,k=column; j<=base_width-LBS && k<=tar_width-LBS; j++,k++,len++)
                    {
                        if(memcmp(base_jbarray[y][j], tar_jbarray[row][k], WINSIZE))
                            break;
                    }
                    if(len > lenmax)
                    {
                        lenmax = len;
                        xmax = x;
                        ymax = y;
                    }
                    /*  The same position is most likely to be the best 
                     *  matching position.  */
                    if(x==column && y==row && lenmax>0)
                        break;
                    /*------------------------------------------------*/
                    d_item = d_item->next;
                }
                if(lenmax)
                {
                    g_array_append_val(cpx,xmax);
                    g_array_append_val(cpy,ymax);
                    g_array_append_val(cpl,lenmax);
                    g_array_append_val(inl,in_len);
                    in_len = 0;
                    column += lenmax;
                }
                else 
                {
                    g_ptr_array_add(inp, data_ptr);
                    in_len ++;
                    column ++;
                }
            }
            else 
            {
                g_ptr_array_add(inp, data_ptr);
                in_len ++;
                column ++;
            }
        }
        if(in_len)
            g_array_append_val(inl,in_len);
    }
}

static void get_instructions(dedupResPtr dedupPtr, jpeg_coe_ptr base, jpeg_coe_ptr target
    #ifdef THREAD_OPTI
    , GHashTable **subBlockTab
    #endif
)
{
    #ifndef THREAD_OPTI
    GHashTable  **subBlockTab = create_block_index(base);
    #endif

    GArray      *cpx[3*SUB_THREAD_NUM], *cpy[3*SUB_THREAD_NUM], *cpl[3*SUB_THREAD_NUM], 
                *inl[3*SUB_THREAD_NUM], *inp[3*SUB_THREAD_NUM];
    uint64_t    tar_width[3], tar_height[3], base_width[3], base_height[3];

    for(int i=0; i<3*SUB_THREAD_NUM; i++)
    {
        cpx[i]  =   g_array_new(FALSE, FALSE, sizeof(COPY_X));
        cpy[i]  =   g_array_new(FALSE, FALSE, sizeof(COPY_Y));
        cpl[i]  =   g_array_new(FALSE, FALSE, sizeof(COPY_L));
        inl[i]  =   g_array_new(FALSE, FALSE, sizeof(INSERT_L));
        inp[i]  =   g_array_new(FALSE, FALSE, sizeof(uint8_t*));
    }

    for(int i=0; i<3; i++)
    {
        tar_width[i]    =   target->imgSize[i*2];
        tar_height[i]   =   target->imgSize[i*2+1];
        base_width[i]   =   base->imgSize[i*2];
        base_height[i]  =   base->imgSize[i*2+1];
    }

    uint8_t     *tar_p[3], *base_p[3];
    tar_p[0]    =   target->data;
    base_p[0]   =   base->data;
    for(int i=1; i<3; i++)
    {
        tar_p[i]    =   tar_p[i-1] + tar_width[i-1]*tar_height[i-1]*sizeof(JBLOCK);
        base_p[i]   =   base_p[i-1] + base_width[i-1]*base_height[i-1]*sizeof(JBLOCK);
    }

    JBLOCKARRAY tar_jbarray[3], base_jbarray[3];
    JBLOCKROW   tarrow0[tar_height[0]], baserow0[base_height[0]];
    for(int j=0; j<tar_height[0]; j++, tar_p[0]+=sizeof(JBLOCK)*tar_width[0])
        tarrow0[j]   =   (JBLOCKROW)tar_p[0];
    for(int j=0; j<base_height[0]; j++, base_p[0]+=sizeof(JBLOCK)*base_width[0])
        baserow0[j]  =   (JBLOCKROW)base_p[0];
    tar_jbarray[0]  =   tarrow0;
    base_jbarray[0] =   baserow0;
    JBLOCKROW   tarrow1[tar_height[1]], baserow1[base_height[1]];
    for(int j=0; j<tar_height[1]; j++, tar_p[1]+=sizeof(JBLOCK)*tar_width[1])
        tarrow1[j]   =   (JBLOCKROW)tar_p[1];
    for(int j=0; j<base_height[1]; j++, base_p[1]+=sizeof(JBLOCK)*base_width[1])
        baserow1[j]  =   (JBLOCKROW)base_p[1];
    tar_jbarray[1]  =   tarrow1;
    base_jbarray[1] =   baserow1;
    JBLOCKROW   tarrow2[tar_height[2]], baserow2[base_height[2]];
    for(int j=0; j<tar_height[2]; j++, tar_p[2]+=sizeof(JBLOCK)*tar_width[2])
        tarrow2[j]   =   (JBLOCKROW)tar_p[2];
    for(int j=0; j<base_height[2]; j++, base_p[2]+=sizeof(JBLOCK)*base_width[2])
        baserow2[j]  =   (JBLOCKROW)base_p[2];
    tar_jbarray[2]  =   tarrow2;
    base_jbarray[2] =   baserow2;

    pthread_t   p_id[3*SUB_THREAD_NUM];
    void        **arg[3*SUB_THREAD_NUM];
    for(int i=0; i<3; i++)
    {
        uint64_t    ave_height  =   tar_height[i]/SUB_THREAD_NUM;
        for(int j=0; j<SUB_THREAD_NUM; j++)
        {
            uint64_t    id  =   i*SUB_THREAD_NUM+j;
            arg[id]     =   (void**)malloc(sizeof(void*)*12);
            arg[id][0]  =   cpx[id];
            arg[id][1]  =   cpy[id];
            arg[id][2]  =   cpl[id];
            arg[id][3]  =   inl[id];
            arg[id][4]  =   inp[id];
            arg[id][5]  =   tar_jbarray[i];
            arg[id][6]  =   base_jbarray[i];
            arg[id][7]  =   (void*)tar_width[i];
            arg[id][8]  =   (void*)base_width[i];
            arg[id][9]  =   (void*)(uint64_t)(j*ave_height);
            arg[id][10] =   (void*)(uint64_t)((j==SUB_THREAD_NUM-1) ? (tar_height[i]-LBS) : ((j+1)*ave_height-1));
            arg[id][11] =   subBlockTab[i];

            pthread_create(&p_id[id], NULL, get_instructions_thread, (void*)arg[id]);
        }
    }

    for(int i=0; i<3*SUB_THREAD_NUM; i++)
    {
        pthread_join(p_id[i], NULL);
        free(arg[i]);
    }

    dedupPtr->y_counter =   0;
    dedupPtr->u_counter =   0;
    dedupPtr->v_counter =   0;
    #ifdef JPEG_SEPA_COMP
    dedupPtr->p_counter[0]  =   0;
    dedupPtr->p_counter[1]  =   0;
    dedupPtr->p_counter[2]  =   0;
    #endif
    for(int i=0; i<SUB_THREAD_NUM; i++)
    {
        dedupPtr->y_counter +=   inl[0*SUB_THREAD_NUM+i]->len;
        dedupPtr->u_counter +=   inl[1*SUB_THREAD_NUM+i]->len;
        dedupPtr->v_counter +=   inl[2*SUB_THREAD_NUM+i]->len;
        #ifdef JPEG_SEPA_COMP
        dedupPtr->p_counter[0]  +=  inp[0*SUB_THREAD_NUM+i]->len;
        dedupPtr->p_counter[1]  +=  inp[1*SUB_THREAD_NUM+i]->len;
        dedupPtr->p_counter[2]  +=  inp[2*SUB_THREAD_NUM+i]->len;
        #endif
    }

    for(int i=1; i<3*SUB_THREAD_NUM; i++)
    {
        g_array_append_vals(cpx[0], cpx[i]->data, cpx[i]->len);
        g_array_free(cpx[i], TRUE);
        g_array_append_vals(cpy[0], cpy[i]->data, cpy[i]->len);
        g_array_free(cpy[i], TRUE);
        g_array_append_vals(cpl[0], cpl[i]->data, cpl[i]->len);
        g_array_free(cpl[i], TRUE);
        g_array_append_vals(inl[0], inl[i]->data, inl[i]->len);
        g_array_free(inl[i], TRUE);
        g_array_append_vals(inp[0], inp[i]->data, inp[i]->len);
        g_array_free(inp[i], TRUE);
    }

    for(int i=0; i<3; i++)
        g_hash_table_destroy(subBlockTab[i]);
    free(subBlockTab);

    dedupPtr->copy_x    =   cpx[0];
    dedupPtr->copy_y    =   cpy[0];
    dedupPtr->copy_l    =   cpl[0];
    dedupPtr->insert_l  =   inl[0];
    dedupPtr->insert_p  =   inp[0];
}
#endif

dedupResPtr dedup_a_single_img(detectionDataPtr detectPtr)
{
    imagePtr        baseImage   =   (imagePtr)detectPtr->base->data,
                    targetImage =   (imagePtr)detectPtr->target->data;
    jpeg_coe_ptr    baseCoe     =   baseImage->decdData->targetInfo->coe,
                    targetCoe   =   targetImage->decdData->targetInfo->coe;
    dedupResPtr     dedupPtr    =   (dedupResPtr)malloc(sizeof(dedupResNode));
    #ifdef HEADER_DELTA
    uint8_t     *buffer =   (uint8_t*)malloc(targetCoe->headerSize);
    uint64_t    delSize;
    xd3_encode_memory(targetCoe->header,
                    targetCoe->headerSize,
                    baseCoe->header,
                    baseCoe->headerSize,
                    buffer,
                    &delSize,
                    targetCoe->headerSize,
                    1);
    dedupPtr->header    =   buffer;
    dedupPtr->headerSize=   delSize;
    #else
    dedupPtr->header    =   targetCoe->header;
    dedupPtr->headerSize=   targetCoe->headerSize;
    #endif
    dedupPtr->baseName  =   baseImage->decdData->rawData->name;
    dedupPtr->name      =   targetImage->decdData->rawData->name;
    dedupPtr->imgSize[0]=   targetCoe->imgSize[0];
    dedupPtr->imgSize[1]=   targetCoe->imgSize[1];
    dedupPtr->imgSize[2]=   targetCoe->imgSize[2];
    dedupPtr->imgSize[3]=   targetCoe->imgSize[3];
    dedupPtr->ffxx      =   targetImage->decdData->targetInfo->ffxx;
    dedupPtr->xx        =   targetImage->decdData->targetInfo->xx;
    dedupPtr->node      =   detectPtr->target;

    get_instructions(dedupPtr, baseCoe, targetCoe
        #ifdef THREAD_OPTI
        , detectPtr->subBlockTab
        #endif
    );

    return dedupPtr;
}

/*---------------------------------------------------------------------------------------*/

de_dedupPtr de_dedup_a_single_img(de_readPtr decodePtr, jpeg_coe_ptr base)
{
    uint32_t    totalSize   =   decodePtr->sizes[0]*decodePtr->sizes[1]+decodePtr->sizes[2]*decodePtr->sizes[3]*2;
    uint8_t     *dataPtr    =   (uint8_t*)malloc(sizeof(JBLOCK)*totalSize);
    INSERT_L    *insert_l   =   (INSERT_L*)decodePtr->in_l;
    COPY_X      *copy_x     =   (COPY_X*)decodePtr->x;
    COPY_Y      *copy_y     =   (COPY_Y*)decodePtr->y;
    COPY_L      *copy_l     =   (COPY_L*)decodePtr->cp_l;
    uint8_t     *inp        =   decodePtr->in_d;
    uint8_t     *base_p     =   base->data;
    jvirt_barray_ptr   *coe =   (jvirt_barray_ptr*)malloc(sizeof(jvirt_barray_ptr)*3);
    int         i, j, k;
    COPY_X      tar_width, x;
    COPY_Y      tar_height, y;

    for(i=0;i<3;i++)
        coe[i] = (jvirt_barray_ptr)g_malloc0(sizeof(struct jvirt_barray_control));

    for(i=0; i<3; i++)
    {
        if(i == 0)
        {
            tar_width   =   decodePtr->sizes[0];
            tar_height  =   decodePtr->sizes[1];
        }
        else 
        {
            tar_width   =   decodePtr->sizes[2];
            tar_height  =   decodePtr->sizes[3];
        }
        JBLOCKROW   *jbrow  =   (JBLOCKROW*)malloc(sizeof(JBLOCKROW)*tar_height);
        uint8_t     *coePtr =   dataPtr;
        for(j=0; j<tar_height; j++, coePtr+=tar_width*sizeof(JBLOCK))
            jbrow[j]    =   (JBLOCKROW)coePtr;
        coe[i]->mem_buffer      =   (JBLOCKARRAY)jbrow;
        coe[i]->blocksperrow    =   tar_width;
        coe[i]->rows_in_array   =   tar_height;
        coe[i]->maxaccess       =   1;
        coe[i]->rows_in_mem     =   coe[i]->rows_in_array;
        coe[i]->rowsperchunk    =   coe[i]->rows_in_array;
        coe[i]->first_undef_row =   coe[i]->rows_in_array;
        coe[i]->pre_zero        =   1;
        coe[i]->dirty           =   1;

        uint32_t    base_w  =   base->imgSize[i*2],
                    base_h  =   base->imgSize[i*2+1];
        JBLOCKROW   baserow[base_h];
        for(j=0; j<base_h; j++, base_p+=sizeof(JBLOCK)*base_w)
            baserow[j] = (JBLOCKROW)base_p;
        JBLOCKARRAY jbarray =   baserow;
        uint32_t    ready   =   0;

        for(j=0; j<decodePtr->sizes[4+i]; j++)
        {
            INSERT_L    inlen   =   *insert_l++;
            if(inlen)
            {
                uint32_t    dataLen =   inlen * sizeof(JBLOCK);
                memcpy(dataPtr, inp, dataLen);
                dataPtr +=  dataLen;
                inp     +=  dataLen;
                ready   +=  inlen;
            }
            if(ready < tar_width)
            {
                y   =   *copy_y++;
                x   =   *copy_x++;
                COPY_L  cplen = *copy_l++;
                for(k=0; k<cplen; k++, dataPtr+=sizeof(JBLOCK))
                    memcpy(dataPtr, jbarray[y][x+k], sizeof(JBLOCK));
                ready   +=  cplen;
            }
            if(ready == tar_width)
                ready = 0;
        }
    }

    if(coe[0]->blocksperrow > coe[1]->blocksperrow)
    {
        coe[0]->maxaccess = 2;
        if(coe[0]->blocksperrow & 1)
            coe[0]->blocksperrow ++;
    }
    if((coe[0]->rows_in_array > coe[1]->rows_in_array) && (coe[0]->rows_in_array & 1))
        coe[0]->rows_in_array ++;
    coe[2]->next = coe[1];
    coe[1]->next = coe[0];
    coe[0]->next = NULL;

    jpeg_coe_ptr    content_p   =   (jpeg_coe_ptr)malloc(sizeof(jpeg_coe));
    de_dedupPtr     dedupPtr    =   (de_dedupPtr)malloc(sizeof(de_dedupNode));
    content_p->data         =   (uint8_t*)coe[0]->mem_buffer[0];
    content_p->imgSize[0]   =   decodePtr->sizes[0];
    content_p->imgSize[1]   =   decodePtr->sizes[1];
    content_p->imgSize[2]   =   decodePtr->sizes[2];
    content_p->imgSize[3]   =   decodePtr->sizes[3];
    content_p->imgSize[4]   =   decodePtr->sizes[2];
    content_p->imgSize[5]   =   decodePtr->sizes[3];
    #ifdef HEADER_DELTA
    uint8_t     *buffer     =   (uint8_t*)malloc(base->headerSize<<8);
    uint64_t    resSize;
    xd3_decode_memory(decodePtr->header,
                        decodePtr->sizes[7],
                        base->header,
                        base->headerSize,
                        buffer,
                        &resSize,
                        base->headerSize<<8,
                        1);
    content_p->header       =   buffer;
    content_p->headerSize   =   resSize;
    #else
    content_p->header       =   decodePtr->header;
    content_p->headerSize   =   decodePtr->sizes[7];
    #endif
    dedupPtr->coe           =   coe;
    dedupPtr->content       =   content_p;
    dedupPtr->name          =   decodePtr->name;
    dedupPtr->oriPtr        =   decodePtr->basename_and_oriptr;
    dedupPtr->ffxx          =   decodePtr->ffxx;
    dedupPtr->xx            =   decodePtr->xx;

    return  dedupPtr;
}