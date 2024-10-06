

#ifndef _WFTASK_H_
#define _WFTASK_H_

#include "../kernel/CommRequest.h"
#include "../kernel/CommScheduler.h"
#include "../kernel/Communicator.h"
#include "../kernel/ExecRequest.h"
#include "../kernel/Executor.h"
#include "../kernel/IORequest.h"
#include "../kernel/SleepRequest.h"
#include "WFConnection.h"
#include "Workflow.h"
#include <assert.h>
#include <atomic>
#include <errno.h>
#include <functional>
#include <string.h>
#include <utility>
#include <iostream>
using namespace std;
enum {
  WFT_STATE_UNDEFINED = -1,                 //*未定义
  WFT_STATE_SUCCESS = CS_STATE_SUCCESS,     //*成功
  WFT_STATE_TOREPLY = CS_STATE_TOREPLY,     //*用于server端
  WFT_STATE_NOREPLY = CS_STATE_TOREPLY + 1, //*用于server端
  WFT_STATE_SYS_ERROR = CS_STATE_ERROR,     //*DNS解析出错
  WFT_STATE_DNS_ERROR = 66,                 //*DNS错误,用于客户端
  WFT_STATE_TASK_ERROR = 67,                //*任务出错
  WFT_STATE_ABORTED = CS_STATE_STOPPED
};

template <class REQ, class RESP> class WFNetworkTask : public CommRequest {
public:
  //*用于客户端,因为服务端的所有操作都是被动的
  void start() {
    assert(!series_of(this));
    Workflow::start_series_work(this, nullptr);
  }

  //*用于客户端,因为服务端的所有操作都是被动的
  void dismiss() {
    assert(!series_of(this));
    delete this;
  }

public:
  //*获取请求
  REQ *get_req() { return &this->req; }
  //*获取响应
  RESP *get_resp() { return &this->resp; }

public:
  void *user_data;

public:
  int get_state() const { return this->state; }
  int get_error() const { return this->error; }

  //*当错误为ETIMEOUT时调用，返回值：
  //*TOR_NOT_TIMEOUT、TOR_WAIT_TIMEOUT、TOR_CONNECT_TIMEOUT，
  //*TOR_TRANSMIT_TIMEOUT（发送或接收）
  int get_timeout_reason() const { return this->timeout_reason; }

  //*仅在回调或服务器进程中调用
  long long get_task_seq() const {
    if (!this->target) {
      errno = ENOTCONN;
      return -1;
    }

    return this->get_seq();
  }

  int get_peer_addr(struct sockaddr *addr, socklen_t *addrlen) const;

  virtual WFConnection *get_connection() const = 0;

public:
  //*全部以毫秒为单位。timeout==-1表示无限制
  void set_send_timeout(int timeout) { this->send_timeo = timeout; }
  void set_receive_timeout(int timeout) { this->receive_timeo = timeout; }
  void set_keep_alive(int timeout) { this->keep_alive_timeo = timeout; }
  void set_watch_timeout(int timeout) { this->watch_timeo = timeout; }

public:
  //*不回复这个请求
  void noreply() {
    if (this->state == WFT_STATE_TOREPLY)
      this->state = WFT_STATE_NOREPLY;
  }

  //*同步推送回复数据
  virtual int push(const void *buf, size_t size) {
    if (this->state != WFT_STATE_TOREPLY && this->state != WFT_STATE_NOREPLY) {
      errno = ENOENT;
      return -1;
    }

    return this->scheduler->push(buf, size, this);
  }

  //*在回复之前检查连接是否已关闭。
  //*在回调函数中始终返回“true”
  bool closed() const {
    switch (this->state) {
    case WFT_STATE_UNDEFINED:
      return false;
    case WFT_STATE_TOREPLY:
    case WFT_STATE_NOREPLY:
      return !this->target->has_idle_conn();
    default:
      return true;
    }
  }

public:
  void set_callback(std::function<void(WFNetworkTask<REQ, RESP> *)> cb) {
    this->callback = std::move(cb);
  }

protected:
  virtual int send_timeout() { return this->send_timeo; }
  virtual int receive_timeout() { return this->receive_timeo; }
  virtual int keep_alive_timeout() { return this->keep_alive_timeo; }
  virtual int first_timeout() { return this->watch_timeo; }

protected:
  int send_timeo;                                           //*发送超时
  int receive_timeo;                                        //*接收超时
  int keep_alive_timeo;                                     //*超时
  int watch_timeo;                                          //*监听超时
  REQ req;                                                  //*请求
  RESP resp;                                                //*响应
  std::function<void(WFNetworkTask<REQ, RESP> *)> callback; //*回调函数

protected:
  WFNetworkTask(CommSchedObject *object, CommScheduler *scheduler,
                std::function<void(WFNetworkTask<REQ, RESP> *)> &&cb)
      : CommRequest(object, scheduler), callback(std::move(cb)) {
    this->send_timeo = -1;
    this->receive_timeo = -1;
    this->keep_alive_timeo = 0;
    this->watch_timeo = 0;
    this->target = NULL;
    this->timeout_reason = TOR_NOT_TIMEOUT;
    this->user_data = NULL;
    this->state = WFT_STATE_UNDEFINED;
    this->error = 0;
  }

  virtual ~WFNetworkTask() {}
};

//*定时器任务
class WFTimerTask : public SleepRequest {
public:
  void start() {
    assert(!series_of(this));
    Workflow::start_series_work(this, nullptr);
  }
  void dismiss() {
    assert(!series_of(this));
    delete this;
  }
public:
  void *user_data;

public:
  int get_state() const { return this->state; }
  int get_error() const { return this->error; }

public:
  void set_callback(std::function<void(WFTimerTask *)> cb) {
    this->callback = std::move(cb);
  }

protected:
  virtual SubTask *done() {
    SeriesWork *series = series_of(this);

    if (this->callback)
      this->callback(this);

    delete this;
    return series->pop();
  }
protected:
  std::function<void(WFTimerTask *)> callback;
public:
  WFTimerTask(CommScheduler *scheduler, std::function<void(WFTimerTask *)> cb)
      : SleepRequest(scheduler), callback(std::move(cb)) {
    this->user_data = NULL;
    this->state = WFT_STATE_UNDEFINED;
    this->error = 0;
  }

protected:
  virtual ~WFTimerTask() {}
};



#include "./WFTask.inl"

#endif
