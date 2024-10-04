/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-30 10:33:17
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-09-30 13:27:58
 * @FilePath     : /myworkflow/src/factory/Workflow.cc
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */

#include "Workflow.h"
#include <assert.h>
#include <functional>
#include <mutex>
#include <stddef.h>
#include <string.h>
#include <utility>

SeriesWork::SeriesWork(SubTask *first, series_callback_t &&cb)
    : callback(std::move(cb)) {
  this->queue = this->buf; //*将队列指向缓冲区
  this->queue_size =
      sizeof this->buf / sizeof *this->buf; //*将缓冲区大小设置 为队列的大小
  this->front = 0;                          //*索引
  this->back = 0;                           //*索引
  this->canceled = false;
  this->finished = false;
  assert(!series_of(first)); //*为空
  first->set_pointer(this);  //*为空,所以设置为当前
  this->first = first;       //*第一个任务
  this->last = NULL;
  this->context = NULL;
  this->in_parallel = NULL;
}

SeriesWork::~SeriesWork() {
  if (this->queue != this->buf)
    delete[] this->queue;
}

void SeriesWork::dismiss_recursive() {
  SubTask *task = first;

  this->callback = nullptr;
  do {
    delete task;
    task = this->pop_task(); //*循环删除任务
  } while (task);
}

//*扩容队列
void SeriesWork::expand_queue() {
  int size = 2 * this->queue_size; //*两倍扩容
  SubTask **queue = new SubTask *[size];
  int i, j;
  i = 0;
  j = this->front; //*当前执行到的任务对应索引
  do {
    queue[i++] = this->queue[j++];
    if (j == this->queue_size)
      j = 0;
  } while (j != this->back);

  if (this->queue != this->buf)
    delete[] this->queue;

  this->queue = queue;     //*将新创建的队列设置为当前队列
  this->queue_size = size; //*重设二倍大小
  this->front = 0;         //*第一个任务的索引
  this->back = i;          //*最后一个任务的索引
}

//*向串行任务中加入任务
void SeriesWork::push_front(SubTask *task) {
  this->mutex.lock();
  //*font==0
  if (--this->front == -1)
    this->front = this->queue_size - 1;
  //*将subtask的所属串行任务指针设置为当前
  task->set_pointer(this);
  this->queue[this->front] = task;
  //*装满了
  if (this->front == this->back)
    //*扩容
    this->expand_queue();

  this->mutex.unlock();
}

//*添加到末尾
void SeriesWork::push_back(SubTask *task) {
  this->mutex.lock();
  task->set_pointer(this);
  this->queue[this->back] = task;
  if (++this->back == this->queue_size)
    this->back = 0;

  if (this->front == this->back)
    this->expand_queue();

  this->mutex.unlock();
}

//*取出一个任务并返回,如果cancel==true,还会递归删除所有任务
SubTask *SeriesWork::pop() {
  bool canceled = this->canceled;
  SubTask *task = this->pop_task();

  if (!canceled)
    return task;

  while (task) {
    delete task;
    task = this->pop_task();
  }

  return NULL;
}

SubTask *SeriesWork::pop_task() {
  SubTask *task;
  this->mutex.lock();
  //*没执行完
  if (this->front != this->back) {
    task = this->queue[this->front];
    //*队头任务为空
    if (++this->front == this->queue_size)
      this->front = 0;
  } else {
    task = this->last;
    this->last = NULL;
  }

  this->mutex.unlock();
  //*任务为空,即执行完成
  if (!task) {
    this->finished = true;
    if (this->callback)
      this->callback(this);
    //*不在并行任务流中
    if (!this->in_parallel)
      delete this;
  }
  return task;
}

//*传入回调函数
ParallelWork::ParallelWork(parallel_callback_t &&cb)
    : ParallelTask(new SubTask *[2 * 4], 0), callback(std::move(cb)) {
  this->buf_size = 4;
  this->all_series = (SeriesWork **)&this->subtasks[this->buf_size];
  this->context = NULL;
}

ParallelWork::ParallelWork(SeriesWork *const all_series[], size_t n,
                           parallel_callback_t &&cb)
    : ParallelTask(new SubTask *[2 * (n > 4 ? n : 4)], n),
      callback(std::move(cb)) {
  size_t i;
  this->buf_size = (n > 4 ? n : 4);
  this->all_series = (SeriesWork **)&this->subtasks[this->buf_size];
  for (i = 0; i < n; i++) {
    assert(!all_series[i]->in_parallel);
    all_series[i]->in_parallel = this;
    this->all_series[i] = all_series[i]; //*串行任务流拷贝进当前并行任务流
    this->subtasks[i] = all_series[i]->first; //*设置初始化函数
  }

  this->context = NULL;
}

//*扩容队列
void ParallelWork::expand_buf() {
  SubTask **buf;//*扩容缓冲区
  size_t size;
  this->buf_size *= 2;
  buf = new SubTask *[2 * this->buf_size];
  size = this->subtasks_nr * sizeof(void *);
  memcpy(buf, this->subtasks, size);
  memcpy(buf + this->buf_size, this->all_series, size);

  delete[] this->subtasks;
  this->subtasks = buf;
  this->all_series = (SeriesWork **)&buf[this->buf_size];
}

void ParallelWork::add_series(SeriesWork *series) {
  if (this->subtasks_nr == this->buf_size)
    this->expand_buf();

  assert(!series->in_parallel);
  series->in_parallel = this;
  this->all_series[this->subtasks_nr] = series;
  this->subtasks[this->subtasks_nr] = series->first;
  this->subtasks_nr++;
}

SubTask *ParallelWork::done() {
  SeriesWork *series = series_of(this);
  size_t i;

  if (this->callback)
    this->callback(this);

  for (i = 0; i < this->subtasks_nr; i++)
    delete this->all_series[i];

  this->subtasks_nr = 0;
  delete this;
  return series->pop();
}

ParallelWork::~ParallelWork() {
  size_t i;

  for (i = 0; i < this->subtasks_nr; i++) {
    this->all_series[i]->in_parallel = NULL;
    this->all_series[i]->dismiss_recursive();
  }

  delete[] this->subtasks;
}
