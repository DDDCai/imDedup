/*
 * @Author: Cai Deng
 * @Date: 2021-01-13 15:28:42
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-14 10:14:28
 * @Description: 
 */
#ifndef _INCLUDE_BUFFER_H_
#define _INCLUDE_BUFFER_H_

#include "idedup.h"

typedef struct buf_node_
{
    void        *data;
    uint64_t    size, link;
    pthread_mutex_t mutex;
    struct buf_node_ *next, *pre;

}   buf_node;

typedef struct 
{
    buf_node        *head, *tail;
    uint64_t        size;
    pthread_mutex_t mutex;

}   Buffer;

void insert_to_buffer(buf_node *item, Buffer *buf, void(*free_func)(void *p));
void move_in_buffer(buf_node *item, Buffer *buf, uint64_t(*data_func)(void *p), void(*free_func)(void *p));

#endif