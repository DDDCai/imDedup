/*
 * @Author: Cai Deng
 * @Date: 2021-01-13 15:08:44
 * @LastEditors: Cai Deng
 * @LastEditTime: 2021-01-13 21:16:24
 * @Description: 
 */

#include "buffer.h"

void insert_to_buffer(buf_node *item, Buffer *buf, void(*free_func)(void *p))
{
    pthread_mutex_lock(&buf->mutex);
    while(buf->size < item->size)
    {
        buf_node    *ptr    =   buf->head;
        buf->head   =   ptr->next;
        buf->head->pre  =   NULL;
        free_func(ptr->data);
        buf->size   +=  ptr->size;
    }
    if(buf->head)
    {
        buf->tail->next =   item;
        item->pre   =   buf->tail;
    }
    else 
    {
        buf->head   =   item;
        item->pre   =   NULL;
    }
    item->next  =   NULL;
    buf->tail   =   item;
    buf->size -= item->size;
    pthread_mutex_unlock(&buf->mutex);
}

void move_in_buffer(buf_node *item, Buffer *buf, uint64_t(*data_func)(void *p), void(*free_func)(void *p))
{
    pthread_mutex_lock(&buf->mutex);
    if(buf->size <= START_TO_MOVE)
    {
        uint64_t extraSize = data_func(item);
        if(extraSize)
        {
            while(buf->size < extraSize)
            {
                buf_node    *ptr    =   buf->head;
                buf->head   =   ptr->next;
                buf->head->pre  =   NULL;
                free_func(ptr->data);
                buf->size   +=  ptr->size;
            }
            buf->size -= extraSize;
            buf->tail->next = item;
            item->pre = buf->tail;
            item->next = NULL;
            buf->tail = item;
        }
        else if(item != buf->tail)
        {
            if(item->pre)
                item->pre->next = item->next;
            else 
                buf->head = item->next;
            item->next->pre = item->pre;
            buf->tail->next = item;
            item->pre = buf->tail;
            item->next = NULL;
            buf->tail = item;
        }
    }
    pthread_mutex_unlock(&buf->mutex);
}