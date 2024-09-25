/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-09 21:43:16
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-09-25 12:32:56
 * @FilePath     : /myworkflow/src/kernel/thrdpool.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */

#ifndef _THRDPOOL_H_
#define _THRDPOOL_H_

#include <stddef.h>

typedef struct __thrdpool thrdpool_t;

struct thrdpool_task {
  void (*routine)(void *); //*函数指针
  void *context;           //*函数上下文
};

#ifdef __cplusplus
extern "C" {
#endif

thrdpool_t *thrdpool_create(size_t nthreads, size_t stacksize);
int thrdpool_schedule(const struct thrdpool_task *task, thrdpool_t *pool);
int thrdpool_in_pool(thrdpool_t *pool);
int thrdpool_increase(thrdpool_t *pool);
int thrdpool_decrease(thrdpool_t *pool);
void thrdpool_exit(thrdpool_t *pool);
void thrdpool_destroy(void (*pending)(const struct thrdpool_task *),
                      thrdpool_t *pool);

#ifdef __cplusplus
}
#endif

#endif
