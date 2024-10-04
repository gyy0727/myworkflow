/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-10-04 21:51:50
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-04 21:51:51
 * @FilePath     : /myworkflow/src/server/WFHttpServer.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */
/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-28 16:33:12
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-04 21:47:21
 * @FilePath     :
 * /myworkflow/root/desktop/opensource/workflowanalysis/src/server/WFHttpServer.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */

#ifndef _WFHTTPSERVER_H_
#define _WFHTTPSERVER_H_

#include "../protocol/HttpMessage.h"
#include "WFServer.h"
#include "../factory/WFTaskFactory.h"
#include <utility>

using http_process_t = std::function<void(WFHttpTask *)>;
using WFHttpServer = WFServer<protocol::HttpRequest, protocol::HttpResponse>;

static constexpr struct WFServerParams HTTP_SERVER_PARAMS_DEFAULT = {
    .transport_type = TT_TCP,
    .max_connections = 2000,
    .peer_response_timeout = 10 * 1000,
    .receive_timeout = -1,
    .keep_alive_timeout = 60 * 1000,
    .request_size_limit = (size_t)-1,
};

template <>
inline WFHttpServer::WFServer(http_process_t proc)
    : WFServerBase(&HTTP_SERVER_PARAMS_DEFAULT), process(std::move(proc)) {}

template <>
inline CommSession *WFHttpServer::new_session(long long seq,
                                              CommConnection *conn) {
  WFHttpTask *task;

  task = WFServerTaskFactory::create_http_task(this, this->process);
  task->set_keep_alive(this->params.keep_alive_timeout);
  task->set_receive_timeout(this->params.receive_timeout);
  task->get_req()->set_size_limit(this->params.request_size_limit);

  return task;
}

#endif
