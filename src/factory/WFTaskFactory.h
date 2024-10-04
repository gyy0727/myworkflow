/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-10-04 13:26:06
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-04 15:22:26
 * @FilePath     : /myworkflow/src/factory/WFTaskFactory.h
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

  Authors: Xie Han (xiehan@sogou-inc.com)
           Wu Jiaxu (wujiaxu@sogou-inc.com)
*/

#ifndef _WFTASKFACTORY_H_
#define _WFTASKFACTORY_H_


#include "../manager/EndpointParams.h"
#include "../protocol/HttpMessage.h"
#include "../util/URIParser.h"
#include "WFTask.h"
#include "Workflow.h"
#include <functional>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <utility>

// Network Client/Server tasks

using WFHttpTask = WFNetworkTask<protocol::HttpRequest, protocol::HttpResponse>;
using http_callback_t = std::function<void(WFHttpTask *)>;



template <class REQ, class RESP> class WFNetworkTaskFactory {
private:
  using T = WFNetworkTask<REQ, RESP>;

public:


public:
  static T *create_server_task(CommService *service,
                               std::function<void(T *)> &process);
};


#include "../manager/EndpointParams.h"
#include "../manager/WFGlobal.h"
#include "../util/URIParser.h"
#include "WFHttpServerTask.h"
#include "WFTask.h"
#include "WFTaskError.h"
#include "Workflow.h"
#include <atomic>
#include <errno.h>
#include <functional>
#include <netdb.h>
#include <stdio.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <utility>


/**********Template Network Factory**********/

template <class REQ, class RESP>
WFNetworkTask<REQ, RESP> *WFNetworkTaskFactory<REQ, RESP>::create_server_task(
    CommService *service,
    std::function<void(WFNetworkTask<REQ, RESP> *)> &proc) {
  return new WFServerTask<REQ, RESP>(service, WFGlobal::get_scheduler(), proc);
}

/**********Server Factory**********/

class WFServerTaskFactory {
public:
  
  static WFHttpTask *create_http_task(CommService *service,
                                      std::function<void(WFHttpTask *)> &proc) {
    return new WFHttpServerTask(service, proc);
  }


};


#endif
