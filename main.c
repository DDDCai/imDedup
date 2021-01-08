/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 08:11:45
 * @LastEditors: Cai Deng
 * @LastEditTime: 2020-12-10 07:38:12
 * @Description: 
 */
#include "idedup.h"

#ifdef PART_TIME
double read_time = 0;
double decode_time = 0;
double detect_time = 0;
double dedup_time = 0;
double rejpeg_time = 0;
double write_time = 0;
#endif

#ifdef DEBUG_2
uint64_t total_count = 0;
uint64_t similar_count = 0;
uint64_t ava_count = 0;
uint64_t receive_count = 0;
pthread_mutex_t ava_mutex;
#endif

/*
 * argv:
 *  1: -c or -d;
 *  2: src folder;
 *  3: dest folder;
 *  4: ref folder.
 */
void main(int argc, char *argv[])
{
    GTimer      *timer = g_timer_new();
    double      time;
    struct      dirent  *entry;
    DIR         *dir;
    char        inPath[MAX_PATH_LEN], outPath[MAX_PATH_LEN];
    #ifdef  CHECK_DECOMPRESS
    char        oriPath[MAX_PATH_LEN];
    #endif
    uint64_t    *result, rawSize = 0, undecodeSize = 0, finalSize = 0;
    #ifdef DEBUG_1
    uint64_t    sizes[8] = {0};
    #endif

    #ifdef DEBUG_2
    pthread_mutex_init(&ava_mutex, NULL);
    #endif

    g_timer_start(timer);

    if(!(dir = opendir(argv[2])))
    {
        printf("fail to open folder %s\n", argv[2]);
        exit(EXIT_FAILURE);
    }
    if(!strcmp(argv[1], "-c"))
    {
        while(entry = readdir(dir))
        {
            if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
                continue ;
            PUT_3_STRS_TOGETHER(inPath,argv[2],"/",entry->d_name);
            PUT_3_STRS_TOGETHER(outPath,argv[3],"/",entry->d_name);
            result  =   idedup_compress(inPath, outPath);
            rawSize +=  result[0];
            undecodeSize    +=  result[1];
            finalSize   +=  result[2];
            #ifdef DEBUG_1
            for(int i=0; i<8; i++)
                sizes[i] += result[3+i];
            #endif

            free(result);
        }
    }
    else if(!strcmp(argv[1], "-d"))
    {
        while(entry = readdir(dir))
        {
            if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
                continue ;
            PUT_3_STRS_TOGETHER(inPath,argv[2],"/",entry->d_name);
            PUT_3_STRS_TOGETHER(outPath,argv[3],"/",entry->d_name);
            #ifdef  CHECK_DECOMPRESS
            PUT_3_STRS_TOGETHER(oriPath,argv[4],"/",entry->d_name);
            #endif
            rawSize +=  idedup_decompress(inPath, outPath
                #ifdef  CHECK_DECOMPRESS
                    , oriPath
                #endif
            );
        }
    }
    else 
    {
        printf("missing arguments (\"-c\" or \"-d\")\n");
        exit(EXIT_FAILURE);
    }
    closedir(dir);

    time = g_timer_elapsed(timer,NULL);
    g_timer_destroy(timer);

    #ifdef DEBUG_2
    pthread_mutex_destroy(&ava_mutex);
    #endif

    printf("----------------------start------------------------\n\n");
    printf("compression ratio   :  %f : 1\n",(double)(rawSize-undecodeSize)/finalSize);
    printf("bandwidth           :  %f MB/s\n", (rawSize-undecodeSize)/time/1024/1024);
    printf("avaliable size      :  %f GB\n", (rawSize-undecodeSize)/1024.0/1024/1024);
    printf("compressed size     :  %f GB\n", finalSize/1024.0/1024/1024);
    printf("unavaliable size    :  %f GB\n", undecodeSize/1024.0/1024/1024);
    printf("time                :  %f s\n", time);
    #ifdef DEBUG_1
    printf("\n");
    for(int i=0; i<8; i++)
        printf("%.2f%%, %.2f MB\n", (float)sizes[i]/sizes[7]*100, (float)sizes[i]/1024/1024);
    #endif
    #ifdef DEBUG_2
    printf("\n");
    printf("total       : %lu\n", total_count);
    printf("avaliable   : %lu\n", ava_count);
    printf("similar     : %lu\n", similar_count);
    printf("receive     : %lu\n", receive_count);
    #endif
    #ifdef PART_TIME
    printf("\n");
    printf("read    : %f s\n", read_time);
    printf("decode  : %f s\n", decode_time);
    printf("detect  : %f s\n", detect_time);
    printf("dedup   : %f s\n", dedup_time);
    printf("rejpeg  : %f s\n", rejpeg_time);
    printf("write   : %f s\n", write_time);
    #endif
    printf("\n-----------------------end-------------------------\n");
}