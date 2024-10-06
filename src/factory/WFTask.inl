/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-30 13:44:19
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-06 13:44:20
 * @FilePath     : /myworkflow/src/factory/WFTask.inl
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */

#include <thread>
template <class REQ, class RESP>
int WFNetworkTask<REQ, RESP>::get_peer_addr(struct sockaddr *addr,
                                            socklen_t *addrlen) const {
  const struct sockaddr *p;
  socklen_t len;

  if (this->target) {
    this->target->get_addr(&p, &len);
    if (*addrlen >= len) {
      memcpy(addr, p, len);
      *addrlen = len;
      return 0;
    }

    errno = ENOBUFS;
  } else
    errno = ENOTCONN;

  return -1;
}

template <class REQ, class RESP>
class WFServerTask : public WFNetworkTask<REQ, RESP> {
protected:
  virtual CommMessageOut *message_out() { return &this->resp; }
  virtual CommMessageIn *message_in() { return &this->req; }
  virtual void handle(int state, int error);

protected:
  virtual WFConnection *get_connection() const {
    if (this->processor.task)
      return (WFConnection *)this->CommSession::get_connection();

    errno = EPERM;
    return NULL;
  }

protected:
  virtual void dispatch() {
    std::cout << "server::dispatch() --- " << std::this_thread::get_id()
              << std::endl;
    if (this->state == WFT_STATE_TOREPLY) {
      /* Enable get_connection() again if the reply() call is success. */
      this->processor.task = this;
      if (this->scheduler->reply(this) >= 0)
        return;

      this->state = WFT_STATE_SYS_ERROR;
      this->error = errno;
      this->processor.task = NULL;
    } else
      this->scheduler->shutdown(this);

    this->subtask_done();
  }

  virtual SubTask *done() {
    SeriesWork *series = series_of(this);

    if (this->callback)
      this->callback(this);

    /* Defer deleting the task. */
    return series->pop();
  }

protected:
  class Processor : public SubTask {
  public:
    Processor(WFServerTask<REQ, RESP> *task,
              std::function<void(WFNetworkTask<REQ, RESP> *)> &proc)
        : process(proc) {
      this->task = task;
    }

    virtual void dispatch() {
      std::cout << "process::dispatch() --- " << std::this_thread::get_id()
                << std::endl;
      this->process(this->task);
      this->task = NULL; /* As a flag. get_conneciton() disabled. */
      this->subtask_done();
    }

    virtual SubTask *done() { return series_of(this)->pop(); }

    std::function<void(WFNetworkTask<REQ, RESP> *)> &process;
    WFServerTask<REQ, RESP> *task;
  } processor;

  class Series : public SeriesWork {
  public:
    Series(WFServerTask<REQ, RESP> *task)
        : SeriesWork(&task->processor, nullptr) {
      this->set_last_task(task);
      this->task = task;
    }

    virtual ~Series() { delete this->task; }

    WFServerTask<REQ, RESP> *task;
  };

public:
  WFServerTask(CommService *service, CommScheduler *scheduler,
               std::function<void(WFNetworkTask<REQ, RESP> *)> &proc)
      : WFNetworkTask<REQ, RESP>(NULL, scheduler, nullptr),
        processor(this, proc) {}

protected:
  virtual ~WFServerTask() {
    if (this->target)
      ((Series *)series_of(this))->task = NULL;
  }
};

template <class REQ, class RESP>
void WFServerTask<REQ, RESP>::handle(int state, int error) {
  std::cout << "server::handle --- " << std::this_thread::get_id() << std::endl;
  if (state == WFT_STATE_TOREPLY) {
    this->state = WFT_STATE_TOREPLY;
    this->target = this->get_target();
    new Series(this);
    this->processor.dispatch();
  } else if (this->state == WFT_STATE_TOREPLY) {
    this->state = state;
    this->error = error;
    if (error == ETIMEDOUT)
      this->timeout_reason = TOR_TRANSMIT_TIMEOUT;

    this->subtask_done();
  } else
    delete this;
}
