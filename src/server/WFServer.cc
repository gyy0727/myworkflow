/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-10-04 21:51:14
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-05 18:46:09
 * @FilePath     : /myworkflow/src/server/WFServer.cc
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

#include "WFServer.h"
#include "../factory/WFConnection.h"
#include "../kernel/CommScheduler.h"
#include "../manager/EndpointParams.h"
#include "../manager/WFGlobal.h"
#include <atomic>
#include <condition_variable>
#include <errno.h>
#include <mutex>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT_STR_MAX 5

class WFServerConnection : public WFConnection {
public:
  WFServerConnection(std::atomic<size_t> *conn_count) {
    this->conn_count = conn_count;
  }

  virtual ~WFServerConnection() { (*this->conn_count)--; }

private:
  std::atomic<size_t> *conn_count;
};

//*初始化各种参数,并创建scheduler
int WFServerBase::init(const struct sockaddr *bind_addr, socklen_t addrlen) {
  int timeout = this->params.peer_response_timeout;

  if (this->params.receive_timeout >= 0) {
    if ((unsigned int)timeout > (unsigned int)this->params.receive_timeout)
      timeout = this->params.receive_timeout;
  }

  if (this->CommService::init(bind_addr, addrlen, -1, timeout) < 0)
    return -1;

  this->scheduler = WFGlobal::get_scheduler();
  return 0;
}

int WFServerBase::create_listen_fd() {
  if (this->listen_fd < 0) {
    const struct sockaddr *bind_addr;
    socklen_t addrlen;
    int type, protocol;
    int reuse = 1;

    switch (this->params.transport_type) {
    case TT_TCP:
      type = SOCK_STREAM;
      protocol = 0;
      break;
    case TT_UDP:
      type = SOCK_DGRAM;
      protocol = 0;
      break;
#ifdef IPPROTO_SCTP
    case TT_SCTP:
      type = SOCK_STREAM;
      protocol = IPPROTO_SCTP;
      break;
#endif
    default:
      errno = EPROTONOSUPPORT;
      return -1;
    }

    this->get_addr(&bind_addr, &addrlen);
    this->listen_fd = socket(bind_addr->sa_family, type, protocol);
    if (this->listen_fd >= 0) {
      setsockopt(this->listen_fd, SOL_SOCKET, SO_REUSEADDR, &reuse,
                 sizeof(int));
    }
  } else
    this->listen_fd = dup(this->listen_fd);

  return this->listen_fd;
}

WFConnection *WFServerBase::new_connection(int accept_fd) {
  std::cout << "server::connection()" << std::endl;
  if (++this->conn_count <= this->params.max_connections ||
      this->drain(1) == 1) {
    int reuse = 1;
    setsockopt(accept_fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(int));
    return new WFServerConnection(&this->conn_count);
  }

  this->conn_count--;
  errno = EMFILE;
  return NULL;
}

void WFServerBase::delete_connection(WFConnection *conn) {
  delete (WFServerConnection *)conn;
}

void WFServerBase::handle_unbound() {
  this->mutex.lock();
  this->unbind_finish = true;
  this->cond.notify_one();
  this->mutex.unlock();
}

int WFServerBase::start(const struct sockaddr *bind_addr, socklen_t addrlen) {
  if (this->init(bind_addr, addrlen) >= 0) {
    //*开启监听
    if (this->scheduler->bind(this) >= 0)
      return 0;

    this->deinit();
  }

  this->listen_fd = -1;
  return -1;
}

int WFServerBase::start(int family, const char *host, unsigned short port) {
  struct addrinfo hints = {
      .ai_flags = AI_PASSIVE,
      .ai_family = family,
      .ai_socktype = SOCK_STREAM,
  };
  struct addrinfo *addrinfo;
  char port_str[PORT_STR_MAX + 1];
  int ret;

  snprintf(port_str, PORT_STR_MAX + 1, "%d", port);
  ret = getaddrinfo(host, port_str, &hints, &addrinfo);
  if (ret == 0) {
    ret = start(addrinfo->ai_addr, (socklen_t)addrinfo->ai_addrlen);
    freeaddrinfo(addrinfo);
  } else {
    if (ret != EAI_SYSTEM)
      errno = EINVAL;
    ret = -1;
  }

  return ret;
}

int WFServerBase::serve(int listen_fd) {
  struct sockaddr_storage ss;
  socklen_t len = sizeof ss;

  if (getsockname(listen_fd, (struct sockaddr *)&ss, &len) < 0)
    return -1;

  this->listen_fd = listen_fd;
  return start((struct sockaddr *)&ss, len);
}

void WFServerBase::shutdown() {
  this->listen_fd = -1;
  this->scheduler->unbind(this);
}

void WFServerBase::wait_finish() {
  std::unique_lock<std::mutex> lock(this->mutex);

  while (!this->unbind_finish)
    this->cond.wait(lock);

  this->deinit();
  this->unbind_finish = false;
  lock.unlock();
}
