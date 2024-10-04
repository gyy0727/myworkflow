/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-10-04 13:24:28
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-04 13:24:29
 * @FilePath     : /myworkflow/src/factory/WFResourcePool.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */
/*
  Copyright (c) 2021 Sogou, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Authors: Li Yingxin (liyingxin@sogou-inc.com)
           Xie Han (xiehan@sogou-inc.com)
*/

#ifndef _WFRESOURCEPOOL_H_
#define _WFRESOURCEPOOL_H_

#include "WFTask.h"
#include "../kernel/list.h"
#include <mutex>

class WFResourcePool {
public:
  WFConditional *get(SubTask *task, void **resbuf);
  WFConditional *get(SubTask *task);
  void post(void *res);

public:
  struct Data {
    void *pop() { return this->pool->pop(); }
    void push(void *res) { this->pool->push(res); }

    void **res;
    long value;
    size_t index;
    struct list_head wait_list;
    std::mutex mutex;
    WFResourcePool *pool;
  };

protected:
  virtual void *pop() { return this->data.res[this->data.index++]; }

  virtual void push(void *res) { this->data.res[--this->data.index] = res; }

protected:
  struct Data data;

private:
  void create(size_t n);

public:
  WFResourcePool(void *const *res, size_t n);
  WFResourcePool(size_t n);
  virtual ~WFResourcePool() { delete[] this->data.res; }
};

#endif
