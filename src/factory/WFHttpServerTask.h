/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-10-04 13:23:41
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-04 13:23:42
 * @FilePath     : /myworkflow/src/factory/WFHttpServerTask.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */
/*
  Copyright (c) 2023 Sogou, Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  Authors: Xie Han (xiehan@sogou-inc.com)
*/

#ifndef _WFHTTPSERVERTASK_H_
#define _WFHTTPSERVERTASK_H_

#include "../protocol/HttpMessage.h"
#include "../manager/WFGlobal.h"
#include "WFTask.h"

class WFHttpServerTask
    : public WFServerTask<protocol::HttpRequest, protocol::HttpResponse> {
private:
  using TASK = WFNetworkTask<protocol::HttpRequest, protocol::HttpResponse>;

public:
  WFHttpServerTask(CommService *service, std::function<void(TASK *)> &proc)
      : WFServerTask(service, WFGlobal::get_scheduler(), proc),
        req_is_alive_(false), req_has_keep_alive_header_(false) {}

protected:
  virtual void handle(int state, int error);
  virtual CommMessageOut *message_out();

protected:
  bool req_is_alive_;
  bool req_has_keep_alive_header_;
  std::string req_keep_alive_;
};

#endif
