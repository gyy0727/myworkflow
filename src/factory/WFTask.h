

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

//*输入输出,普通的线程任务
template <class INPUT, class OUTPUT> class WFThreadTask : public ExecRequest {
public:
  //*开启执行任务流
  void start() {
    assert(!series_of(this));
    Workflow::start_series_work(this, nullptr);
  }
  //*释放 资源
  void dismiss() {
    assert(!series_of(this));
    delete this;
  }

public:
  //*获取输入
  INPUT *get_input() { return &this->input; }
  //*获取输出
  OUTPUT *get_output() { return &this->output; }

public:
  void *user_data;

public:
  //*执行状态
  int get_state() const { return this->state; }
  //*执行的错误码
  int get_error() const { return this->error; }

public:
  //*设置回调函数
  void set_callback(std::function<void(WFThreadTask<INPUT, OUTPUT> *)> cb) {
    this->callback = std::move(cb);
  }

protected:
  //*当前任务运行完,返回下一个任务
  virtual SubTask *done() {
    SeriesWork *series = series_of(this);

    if (this->callback)
      this->callback(this);

    delete this;
    return series->pop();
  }

protected:
  INPUT input;
  OUTPUT output;
  std::function<void(WFThreadTask<INPUT, OUTPUT> *)> callback;

public:
  WFThreadTask(ExecQueue *queue, Executor *executor,
               std::function<void(WFThreadTask<INPUT, OUTPUT> *)> &&cb)
      : ExecRequest(queue, executor), callback(std::move(cb)) {
    this->user_data = NULL;
    this->state = WFT_STATE_UNDEFINED;
    this->error = 0;
  }

protected:
  virtual ~WFThreadTask() {}
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


//*文件任务
template <class ARGS> class WFFileTask : public IORequest {
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
  ARGS *get_args() { return &this->args; }

  long get_retval() const {
    if (this->state == WFT_STATE_SUCCESS)
      return this->get_res();
    else
      return -1;
  }

public:
  void *user_data;

public:
  int get_state() const { return this->state; }
  int get_error() const { return this->error; }

public:
  void set_callback(std::function<void(WFFileTask<ARGS> *)> cb) {
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
  ARGS args;
  std::function<void(WFFileTask<ARGS> *)> callback;

public:
  WFFileTask(IOService *service, std::function<void(WFFileTask<ARGS> *)> &&cb)
      : IORequest(service), callback(std::move(cb)) {
    this->user_data = NULL;
    this->state = WFT_STATE_UNDEFINED;
    this->error = 0;
  }

protected:
  virtual ~WFFileTask() {}
};

class WFGenericTask : public SubTask {
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

protected:
  virtual void dispatch() { this->subtask_done(); }

  virtual SubTask *done() {
    SeriesWork *series = series_of(this);
    delete this;
    return series->pop();
  }

protected:
  int state;
  int error;

public:
  WFGenericTask() {
    this->user_data = NULL;
    this->state = WFT_STATE_UNDEFINED;
    this->error = 0;
  }

protected:
  virtual ~WFGenericTask() {}
};

class WFCounterTask : public WFGenericTask {
public:
  virtual void count() {
    if (--this->value == 0) {
      this->state = WFT_STATE_SUCCESS;
      this->subtask_done();
    }
  }

public:
  void set_callback(std::function<void(WFCounterTask *)> cb) {
    this->callback = std::move(cb);
  }

protected:
  virtual void dispatch() { this->WFCounterTask::count(); }

  virtual SubTask *done() {
    SeriesWork *series = series_of(this);

    if (this->callback)
      this->callback(this);

    delete this;
    return series->pop();
  }

protected:
  std::atomic<unsigned int> value;
  std::function<void(WFCounterTask *)> callback;

public:
  WFCounterTask(unsigned int target_value,
                std::function<void(WFCounterTask *)> &&cb)
      : value(target_value + 1), callback(std::move(cb)) {}

protected:
  virtual ~WFCounterTask() {}
};

class WFMailboxTask : public WFGenericTask {
public:
  virtual void send(void *msg) {
    *this->mailbox = msg;
    if (this->flag.exchange(true)) {
      this->state = WFT_STATE_SUCCESS;
      this->subtask_done();
    }
  }

  void **get_mailbox() const { return this->mailbox; }

public:
  void set_callback(std::function<void(WFMailboxTask *)> cb) {
    this->callback = std::move(cb);
  }

protected:
  virtual void dispatch() {
    if (this->flag.exchange(true)) {
      this->state = WFT_STATE_SUCCESS;
      this->subtask_done();
    }
  }

  virtual SubTask *done() {
    SeriesWork *series = series_of(this);

    if (this->callback)
      this->callback(this);

    delete this;
    return series->pop();
  }

protected:
  void **mailbox;
  std::atomic<bool> flag;
  std::function<void(WFMailboxTask *)> callback;

public:
  WFMailboxTask(void **mailbox, std::function<void(WFMailboxTask *)> &&cb)
      : flag(false), callback(std::move(cb)) {
    this->mailbox = mailbox;
  }

  WFMailboxTask(std::function<void(WFMailboxTask *)> &&cb)
      : flag(false), callback(std::move(cb)) {
    this->mailbox = &this->user_data;
  }

protected:
  virtual ~WFMailboxTask() {}
};

class WFSelectorTask : public WFGenericTask {
public:
  virtual int submit(void *msg) {
    void *tmp = NULL;
    int ret = 0;

    if (this->message.compare_exchange_strong(tmp, msg) && msg) {
      ret = 1;
      if (this->flag.exchange(true)) {
        this->state = WFT_STATE_SUCCESS;
        this->subtask_done();
      }
    }

    if (--this->nleft == 0) {
      if (!this->message) {
        this->state = WFT_STATE_SYS_ERROR;
        this->error = ENOMSG;
        this->subtask_done();
      }

      delete this;
    }

    return ret;
  }

  void *get_message() const { return this->message; }

public:
  void set_callback(std::function<void(WFSelectorTask *)> cb) {
    this->callback = std::move(cb);
  }

protected:
  virtual void dispatch() {
    if (this->flag.exchange(true)) {
      this->state = WFT_STATE_SUCCESS;
      this->subtask_done();
    }

    if (--this->nleft == 0) {
      if (!this->message) {
        this->state = WFT_STATE_SYS_ERROR;
        this->error = ENOMSG;
        this->subtask_done();
      }

      delete this;
    }
  }

  virtual SubTask *done() {
    SeriesWork *series = series_of(this);

    if (this->callback)
      this->callback(this);

    return series->pop();
  }

protected:
  std::atomic<void *> message;
  std::atomic<bool> flag;
  std::atomic<size_t> nleft;
  std::function<void(WFSelectorTask *)> callback;

public:
  WFSelectorTask(size_t candidates, std::function<void(WFSelectorTask *)> &&cb)
      : message(NULL), flag(false), nleft(candidates + 1),
        callback(std::move(cb)) {}

protected:
  virtual ~WFSelectorTask() {}
};

class WFConditional : public WFGenericTask {
public:
  virtual void signal(void *msg) {
    *this->msgbuf = msg;
    if (this->flag.exchange(true))
      this->subtask_done();
  }

protected:
  virtual void dispatch() {
    series_of(this)->push_front(this->task);
    this->task = NULL;
    if (this->flag.exchange(true))
      this->subtask_done();
  }

protected:
  std::atomic<bool> flag;
  SubTask *task;
  void **msgbuf;

public:
  WFConditional(SubTask *task, void **msgbuf) : flag(false) {
    this->task = task;
    this->msgbuf = msgbuf;
  }

  WFConditional(SubTask *task) : flag(false) {
    this->task = task;
    this->msgbuf = &this->user_data;
  }

protected:
  virtual ~WFConditional() { delete this->task; }
};

class WFGoTask : public ExecRequest {
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
  void set_callback(std::function<void(WFGoTask *)> cb) {
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
  std::function<void(WFGoTask *)> callback;

public:
  WFGoTask(ExecQueue *queue, Executor *executor)
      : ExecRequest(queue, executor) {
    this->user_data = NULL;
    this->state = WFT_STATE_UNDEFINED;
    this->error = 0;
  }

protected:
  virtual ~WFGoTask() {}
};

class WFRepeaterTask : public WFGenericTask {
public:
  void set_create(std::function<SubTask *(WFRepeaterTask *)> create) {
    this->create = std::move(create);
  }

  void set_callback(std::function<void(WFRepeaterTask *)> cb) {
    this->callback = std::move(cb);
  }

protected:
  virtual void dispatch() {
    SubTask *task = this->create(this);

    if (task) {
      series_of(this)->push_front(this);
      series_of(this)->push_front(task);
    } else
      this->state = WFT_STATE_SUCCESS;

    this->subtask_done();
  }

  virtual SubTask *done() {
    SeriesWork *series = series_of(this);

    if (this->state != WFT_STATE_UNDEFINED) {
      if (this->callback)
        this->callback(this);

      delete this;
    }

    return series->pop();
  }

protected:
  std::function<SubTask *(WFRepeaterTask *)> create;
  std::function<void(WFRepeaterTask *)> callback;

public:
  WFRepeaterTask(std::function<SubTask *(WFRepeaterTask *)> &&create,
                 std::function<void(WFRepeaterTask *)> &&cb)
      : create(std::move(create)), callback(std::move(cb)) {}

protected:
  virtual ~WFRepeaterTask() {}
};

class WFModuleTask : public ParallelTask, protected SeriesWork {
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
  SeriesWork *sub_series() { return this; }

  const SeriesWork *sub_series() const { return this; }

public:
  void *user_data;

public:
  void set_callback(std::function<void(const WFModuleTask *)> cb) {
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
  SubTask *first;
  std::function<void(const WFModuleTask *)> callback;

public:
  WFModuleTask(SubTask *first, std::function<void(const WFModuleTask *)> &&cb)
      : ParallelTask(&this->first, 1), SeriesWork(first, nullptr),
        callback(std::move(cb)) {
    this->first = first;
    this->set_in_parallel(this);
    this->user_data = NULL;
  }

protected:
  virtual ~WFModuleTask() {
    if (!this->is_finished())
      this->dismiss_recursive();
  }
};

#include "./WFTask.inl"

#endif
