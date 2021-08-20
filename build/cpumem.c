/*
 * @Author: Cai Deng
 * @Date: 2021-03-06 06:46:46
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-03-06 06:47:03
 * @Description: 
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void main(int argc, char *argv[])
{
    FILE    *fp     =   fopen(argv[1], "r");
    char    buffer[1024];
    char    *ptr;
    double  now;
    int     time = 0;
    double  mem = 0;
    int     cpu = 0;

    fgets(buffer,sizeof(buffer),fp);
    ptr = buffer;
    while(*ptr!='m') ptr++;
    ptr ++;
    int preid = atoi(ptr);
    fseek(fp,0,SEEK_SET);

    while(fgets(buffer,sizeof(buffer),fp))
    {
        time ++;
        ptr =   buffer;
        while(*ptr!='m')   ptr++;
        ptr++;
        int id  =   atoi(ptr);
        while(*ptr!=',')    ptr++;
        ptr += 3;
        char    *pre = ptr;
        while(*pre!=',') pre ++;
        pre --;
        if(*pre=='t')
        {
            ptr += 2;
            int left = atoi(ptr);
            now = left*1.024;
        }
        else if(*pre=='g')
        {
            int left = atoi(ptr);
            while(*ptr!='.') ptr++;
            ptr ++;
            int right = atoi(ptr);
            double _right = right/1000.0;
            now = left + _right;
        }
        else if(*pre=='m')
        {
            int left = atoi(ptr);
            while(*ptr!='.') ptr++;
            ptr++;
            int right = atoi(ptr);
            double _right = right/1000.0/1024;
            now = left/1024.0 + _right;
        }
        else
        {
            int left = atoi(ptr);
            now = left/1024.0/1024/1024;
        }

        while(*ptr!=',') ptr++;
        ptr ++;
        int cpunow = atoi(ptr);

        if(id!=preid)
        {
            time --;
            printf("id:%d,mem:%f,cpu:%d\n",preid,mem/time,cpu/time);
            preid = id;
            cpu = 0;
            mem = 0;
            time = 1;
        }
        if(id==preid)
        {
            cpu += cpunow;
            mem += now;
        }
    }
    printf("id:%d,mem:%f,cpu:%d\n",preid,mem/time,cpu/time);

    fclose(fp);
}