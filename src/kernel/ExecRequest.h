/*
 * @Author       : muqiu0614 3155833132@qq.com
 * @Date         : 2024-09-09 21:43:16
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-09-30 13:47:01
 * @FilePath     : /myworkflow/src/kernel/ExecRequest.h
 * @Description  :
 * Copyright (c) 2024 by muqiu0614 email: 3155833132@qq.com, All Rights
 * Reserved.
 */

#ifndef _EXECREQUEST_H_
#define _EXECREQUEST_H_

#include "Executor.h"
#include "SubTask.h"
#include <cerrno>

class ExecRequest : public SubTask, public ExecSession {
public:
  ExecRequest(ExecQueue *queue, Executor *executor) {
    this->executor = executor;
    this->queue = queue;
  }

  ExecQueue *get_request_queue() const { return this->queue; }
  void set_request_queue(ExecQueue *queue) { this->queue = queue; }

public:
  virtual void dispatch() {
    if (this->executor->request(this, this->queue) < 0)
      this->handle(ES_STATE_ERROR, errno);
  }

protected:
  int state;
  int error;

protected:
  ExecQueue *queue;
  Executor *executor;

protected:
  virtual void handle(int state, int error) {
    this->state = state;
    this->error = error;
    this->subtask_done();
  }
};

#endif
