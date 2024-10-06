/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-10-04 21:50:44
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-05 17:56:14
 * @FilePath     : /myworkflow/src/server/WFServer.h
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

#ifndef _WFSERVER_H_
#define _WFSERVER_H_

#include "../factory/WFTaskFactory.h"
#include "../manager/EndpointParams.h"
#include <atomic>
#include <condition_variable>
#include <errno.h>
#include <functional>
#include <mutex>
#include <sys/socket.h>
#include <sys/types.h>

struct WFServerParams {
  enum TransportType transport_type; //*协议类型
  size_t max_connections;
  int peer_response_timeout; /* timeout of each read or write operation */
  int receive_timeout;       /* timeout of receiving the whole message */
  int keep_alive_timeout;
  size_t request_size_limit;
};

static constexpr struct WFServerParams SERVER_PARAMS_DEFAULT = {
    .transport_type = TT_TCP,
    .max_connections = 2000,
    .peer_response_timeout = 10 * 1000,
    .receive_timeout = -1,
    .keep_alive_timeout = 60 * 1000,
    .request_size_limit = (size_t)-1,
};

class WFServerBase : protected CommService {
public:
  WFServerBase(const struct WFServerParams *params) : conn_count(0) {
    this->params = *params;
    this->unbind_finish = false;
    this->listen_fd = -1;
  }

public:
  /* To start a TCP server */

  /* Start on port with IPv4. */
  int start(unsigned short port) { return start(AF_INET, NULL, port); }

  /* Start with family. AF_INET or AF_INET6. */
  int start(int family, unsigned short port) {
    return start(family, NULL, port);
  }

  /* Start with hostname and port. */
  int start(const char *host, unsigned short port) {
    return start(AF_INET, host, port);
  }

  /* Start with family, hostname and port. */
  int start(int family, const char *host, unsigned short port);

  /* Start with binding address. The only necessary start function. */
  int start(const struct sockaddr *bind_addr, socklen_t addrlen);

  /* To start with a specified fd. For graceful restart or SCTP server. */
  int serve(int listen_fd);

  /* stop() is a blocking operation. */
  void stop() {
    this->shutdown();
    this->wait_finish();
  }

  /*无阻塞终止服务器。用于停止多台服务器。
   *通常，调用shutdown（），然后调用wait_finish（）。
   *但实际上wait_finish（）可以在shutdown（）之前调用，甚至在
   *start（）在另一个线程中*/
  void shutdown();
  void wait_finish();

public:
  size_t get_conn_count() const { return this->conn_count; }

  int get_listen_addr(struct sockaddr *addr, socklen_t *addrlen) const {
    if (this->listen_fd >= 0)
      return getsockname(this->listen_fd, addr, addrlen);

    errno = ENOTCONN;
    return -1;
  }

  const struct WFServerParams *get_params() const { return &this->params; }

protected:
  WFServerParams params;

protected:
  virtual int create_listen_fd();
  virtual WFConnection *new_connection(int accept_fd);
  void delete_connection(WFConnection *conn);

private:
  int init(const struct sockaddr *bind_addr, socklen_t addrlen);
  virtual void handle_unbound();

protected:
  std::atomic<size_t> conn_count; //*链接数量

private:
  int listen_fd;      //*监听文件描述符
  bool unbind_finish; //*取消监听
  std::mutex mutex;
  std::condition_variable cond;
  class CommScheduler *scheduler;
};

template <class REQ, class RESP> class WFServer : public WFServerBase {
public:
  WFServer(const struct WFServerParams *params,
           std::function<void(WFNetworkTask<REQ, RESP> *)> proc)
      : WFServerBase(params), process(std::move(proc)) {}

  WFServer(std::function<void(WFNetworkTask<REQ, RESP> *)> proc)
      : WFServerBase(&SERVER_PARAMS_DEFAULT), process(std::move(proc)) {}

protected:
  virtual CommSession *new_session(long long seq, CommConnection *conn);

protected:
  std::function<void(WFNetworkTask<REQ, RESP> *)> process; //*回调函数
};

template <class REQ, class RESP>
CommSession *WFServer<REQ, RESP>::new_session(long long seq,
                                              CommConnection *conn) {
  std::cout << "server::new_session" << std::endl;
  using factory = WFNetworkTaskFactory<REQ, RESP>;
  WFNetworkTask<REQ, RESP> *task;

  task = factory::create_server_task(this, this->process);
  task->set_keep_alive(this->params.keep_alive_timeout);
  task->set_receive_timeout(this->params.receive_timeout);
  task->get_req()->set_size_limit(this->params.request_size_limit);

  return task;
}

#endif
