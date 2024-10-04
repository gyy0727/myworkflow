/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-30 13:34:00
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-09-30 13:37:17
 * @FilePath     : /myworkflow/src/factory/WFConnection.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */
/*
  Copyright (c) 2019 Sogou, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Author: Xie Han (xiehan@sogou-inc.com)
*/

#ifndef _WFCONNECTION_H_
#define _WFCONNECTION_H_

#include "../kernel/Communicator.h"
#include <atomic>
#include <functional>
#include <utility>

class WFConnection : public CommConnection {
public:
  void *get_context() const { return this->context; }

  void set_context(void *context, std::function<void(void *)> deleter) {
    this->context = context;
    this->deleter = std::move(deleter);
  }
  
  void *test_set_context(void *test_context, void *new_context,
                         std::function<void(void *)> deleter) {
    if (this->context.compare_exchange_strong(test_context, new_context)) {
      this->deleter = std::move(deleter);
      return new_context;
    }

    return test_context;
  }

private:
  std::atomic<void *> context;         //*上下文
  std::function<void(void *)> deleter; //*资源删除器

public:
  WFConnection() : context(NULL) {}

protected:
  virtual ~WFConnection() {
    if (this->deleter)
      this->deleter(this->context);
  }
};

#endif
