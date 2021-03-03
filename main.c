/*
 * @Author: Cai Deng
 * @Date: 2020-10-12 08:11:45
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-03-02 16:26:27
 * @Description: 
 */
#include "idedup.h"
#include <getopt.h>

#define COMPRESS 1
#define DECOMPRESS 2

int READ_THREAD_NUM;
int MIDDLE_THREAD_NUM;
int WRITE_THREAD_NUM;

int ROAD_NUM;

int64_t DECODE_BUFFER_SIZE;
int64_t PATCH_SIZE;

int64_t NAME_LIST_MAX;
int64_t READ_LIST_MAX;
int64_t DECD_LIST_MAX;
int64_t DECT_LIST_MAX;
int64_t DEUP_LIST_MAX;
int64_t REJG_LIST_MAX;

uint8_t chunking_mode;
uint8_t in_chaos;

#ifdef PART_TIME
double read_time = 0;
double decode_time = 0;
double detect_time = 0;
double dedup_time = 0;
double rejpeg_time = 0;
double write_time = 0;
#endif


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
    int         option, mode = 0;
    char        *input_path, *output_path, *reference_path;
    static      struct  option  longOptions[]   =   
    {
        {"read_thrd_num", required_argument, NULL, 'a'},
        {"middle_thrd_num", required_argument, NULL, 'b'},
        {"write_thrd_num", required_argument, NULL, 'e'},
        {"input_path", required_argument, NULL, 'f'},
        {"output_path", required_argument, NULL, 'g'},
        {"reference_path", required_argument, NULL, 'h'},
        {"buffer_size", required_argument, NULL, 'i'},
        {"patch_size", required_argument, NULL, 'j'},
        {"name_list", required_argument, NULL, 'k'},
        {"read_list", required_argument, NULL, 'l'},
        {"decd_list", required_argument, NULL, 'm'},
        {"dect_list", required_argument, NULL, 'n'},
        {"deup_list", required_argument, NULL, 'o'},
        {"rejg_list", required_argument, NULL, 'p'},
        {"chunking", required_argument, NULL, 'q'},
        {"road_num", required_argument, NULL, 'r'},
        {"chaos", required_argument, NULL, 's'}
    };

    while((option = getopt_long_only(argc, argv, "cd", longOptions, NULL))!=-1)
    {
        switch (option)
        {
        case 'c':
            mode = COMPRESS;
            break;
        case 'd':
            mode = DECOMPRESS;
            break;
        case 'a':
            READ_THREAD_NUM = atoi(optarg);
            break;
        case 'b':
            MIDDLE_THREAD_NUM = atoi(optarg);
            break;
        case 'e':
            WRITE_THREAD_NUM = atoi(optarg);
            break;
        case 'f':
            input_path = optarg;
            break;
        case 'g':
            output_path = optarg;
            break;
        case 'h':
            reference_path = optarg;
            break;
        case 'i':
            if(*optarg=='G' || *optarg=='g')      DECODE_BUFFER_SIZE = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') DECODE_BUFFER_SIZE = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') DECODE_BUFFER_SIZE = ((int64_t)atoi(optarg+1)) << 10;
            else                                  DECODE_BUFFER_SIZE = (int64_t)atoi(optarg);
            break;
        case 'j':
            if(*optarg=='G' || *optarg=='g')      PATCH_SIZE = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') PATCH_SIZE = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') PATCH_SIZE = ((int64_t)atoi(optarg+1)) << 10;
            else                                  PATCH_SIZE = (int64_t)atoi(optarg);
            break;
        case 'k':
            if(*optarg=='G' || *optarg=='g')      NAME_LIST_MAX = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') NAME_LIST_MAX = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') NAME_LIST_MAX = ((int64_t)atoi(optarg+1)) << 10;
            else                                  NAME_LIST_MAX = (int64_t)atoi(optarg);
            break;
        case 'l':
            if(*optarg=='G' || *optarg=='g')      READ_LIST_MAX = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') READ_LIST_MAX = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') READ_LIST_MAX = ((int64_t)atoi(optarg+1)) << 10;
            else                                  READ_LIST_MAX = (int64_t)atoi(optarg);
            break;
        case 'm':
            if(*optarg=='G' || *optarg=='g')      DECD_LIST_MAX = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') DECD_LIST_MAX = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') DECD_LIST_MAX = ((int64_t)atoi(optarg+1)) << 10;
            else                                  DECD_LIST_MAX = (int64_t)atoi(optarg);
            break;
        case 'n':
            if(*optarg=='G' || *optarg=='g')      DECT_LIST_MAX = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') DECT_LIST_MAX = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') DECT_LIST_MAX = ((int64_t)atoi(optarg+1)) << 10;
            else                                  DECT_LIST_MAX = (int64_t)atoi(optarg);
            break;
        case 'o':
            if(*optarg=='G' || *optarg=='g')      DEUP_LIST_MAX = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') DEUP_LIST_MAX = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') DEUP_LIST_MAX = ((int64_t)atoi(optarg+1)) << 10;
            else                                  DEUP_LIST_MAX = (int64_t)atoi(optarg);
            break;
        case 'p':
            if(*optarg=='G' || *optarg=='g')      REJG_LIST_MAX = ((int64_t)atoi(optarg+1)) << 30;
            else if(*optarg=='M' || *optarg=='m') REJG_LIST_MAX = ((int64_t)atoi(optarg+1)) << 20;
            else if(*optarg=='K' || *optarg=='k') REJG_LIST_MAX = ((int64_t)atoi(optarg+1)) << 10;
            else                                  REJG_LIST_MAX = (int64_t)atoi(optarg);
            break;
        case 'q':
            if(*optarg=='v' || *optarg=='V')        chunking_mode   =   0;
            else                                    chunking_mode   =   1;
            break;
        case 'r':
            ROAD_NUM    =   atoi(optarg);
            break;
        case 's':
            if(*optarg=='y' || *optarg=='Y')     in_chaos   =   1;
            else                                 in_chaos   =   0;
            break;
        default:
            break;
        }
    }

    g_timer_start(timer);

    if(mode == COMPRESS)
    {
        if(!chunking_mode)  READ_THREAD_NUM =   1;  
        /* only when it equals to 1, this system can chunk in variable mode. */
        result  =   idedup_compress(input_path, output_path);
        rawSize +=  result[0];
        undecodeSize    +=  result[1];
        finalSize   +=  result[2];
        #ifdef DEBUG_1
        for(int i=0; i<8; i++)
            sizes[i] += result[3+i];
        #endif

        free(result);
    }
    else if(mode == DECOMPRESS)
    {
        dir =   opendir(input_path);
        while(entry = readdir(dir))
        {
            if(!strcmp(entry->d_name,".") || !strcmp(entry->d_name,".."))
                continue ;
            PUT_3_STRS_TOGETHER(inPath,input_path,"/",entry->d_name);
            PUT_3_STRS_TOGETHER(outPath,output_path,"/",entry->d_name);
            if(access(outPath, 0) < 0)
                mkdir(outPath, 0755);
            #ifdef  CHECK_DECOMPRESS
            // PUT_3_STRS_TOGETHER(oriPath,reference_path,"/",entry->d_name);
            #endif
            rawSize +=  idedup_decompress(inPath, outPath
                #ifdef  CHECK_DECOMPRESS
                    // , oriPath
                    , reference_path
                #endif
            );
        }
        closedir(dir);
    }
    else 
    {
        printf("missing necessary arguments (\"-c\" or \"-d\")\n");
        exit(EXIT_FAILURE);
    }

    time = g_timer_elapsed(timer,NULL);
    g_timer_destroy(timer);

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