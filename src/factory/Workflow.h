/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-30 10:09:25
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-09-30 12:04:31
 * @FilePath     : /myworkflow/src/factory/Workflow.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */
#pragma once
#include "../kernel/SubTask.h"
#include <assert.h>
#include <functional>
#include <mutex>
#include <stddef.h>
#include <utility>

class SeriesWork;
class ParallelWork;
using series_callback_t = std::function<void(const SeriesWork *)>;
using parallel_callback_t = std::function<void(const ParallelWork *)>;

class Workflow {

public:
  //*创建串行任务
  static SeriesWork *create_series_work(SubTask *first,
                                        series_callback_t callback);

  static void start_series_work(SubTask *first, series_callback_t callback);

  static ParallelWork *create_parallel_work(parallel_callback_t callback);

  static ParallelWork *create_parallel_work(SeriesWork *const all_series[],
                                            size_t n,
                                            parallel_callback_t callback);

  static void start_parallel_work(SeriesWork *const all_series[], size_t n,
                                  parallel_callback_t callback);

public:
  static SeriesWork *create_series_work(SubTask *first, SubTask *last,
                                        series_callback_t callback);

  static void start_series_work(SubTask *first, SubTask *last,
                                series_callback_t callback);
};

class SeriesWork {
public:
  //*开始执行任务
  void start() {
    assert(!this->in_parallel);
    this->first->dispatch();
  }

  void dismiss() {
    assert(!this->in_parallel);
    this->dismiss_recursive();
  }

public:
  //*将任务放到队尾
  void push_back(SubTask *task);
  //*将任务放到队头
  void push_front(SubTask *task);

public:
  //*获取上下文
  void *get_context() const { return this->context; }
  //*设置上下文
  void set_context(void *context) { this->context = context; }

public:
  //*取消执行
  virtual void cancel() { this->canceled = true; }
  //*是否已经取消
  bool is_canceled() const { return this->canceled; }
  //*是否已完成
  bool is_finished() const { return this->finished; }

public:
  //*设置回调函数
  void set_callback(series_callback_t callback) {
    this->callback = std::move(callback);
  }

public:
  virtual void *get_specific(const char *key) { return NULL; }

public:
  SubTask *pop();

  SubTask *get_last_task() const { return this->last; }

  void set_last_task(SubTask *last) {
    last->set_pointer(this);
    this->last = last;
  }

  void unset_last_task() { this->last = NULL; }

  const ParallelTask *get_in_parallel() const { return this->in_parallel; }

protected:
  void set_in_parallel(const ParallelTask *task) { this->in_parallel = task; }

  //*释放资源,删除任务队列的任务
  void dismiss_recursive();

protected:
  void *context;              //*callback上下文
  series_callback_t callback; //*执行完任务的回调函数

private:
  SubTask *pop_task(); //*取出队尾任务
  void expand_queue(); //*扩容

private:
  SubTask *buf[4];
  SubTask *first;  //*第一个任务,单独存储,就是初始化函数
  SubTask *last;   //*最后一个任务,单独存储,结束函数
  SubTask **queue; //*任务队列
  int queue_size;  //*任务队列的大小
  int front;       //*头任务,指向queue->size
  int back;        //*尾任务,指向0
  bool canceled;   //*取消
  bool finished;   //*完成
  const ParallelTask *in_parallel; //*是否并行
  std::mutex mutex;                //*互斥锁

protected:
  SeriesWork(SubTask *first, series_callback_t &&callback);
  virtual ~SeriesWork();
  friend class ParallelWork;
  friend class Workflow;
};

//*获得当前subtask所属的串行任务流的指针
static inline SeriesWork *series_of(const SubTask *task) {
  //*将get_pointer的指针强转成series_work
  return (SeriesWork *)task->get_pointer();
}
//*可以通过*subtask对象访问到get_pointer()指针
static inline SeriesWork &operator*(const SubTask &task) {
  return *series_of(&task);
}

//*像series添加subtask任务
static inline SeriesWork &operator<<(SeriesWork &series, SubTask *task) {
  series.push_back(task);
  return series;
}

//*创建串行任务
inline SeriesWork *Workflow::create_series_work(SubTask *first,
                                                series_callback_t callback) {
  return new SeriesWork(first, std::move(callback));
}

//*暂不清楚,好像是创建并执行
inline void Workflow::start_series_work(SubTask *first,
                                        series_callback_t callback) {
  new SeriesWork(first, std::move(callback));
  first->dispatch();
}

//*创建串行任务的同时设置最后一个任务
inline SeriesWork *Workflow::create_series_work(SubTask *first, SubTask *last,
                                                series_callback_t callback) {
  SeriesWork *series = new SeriesWork(first, std::move(callback));
  series->set_last_task(last);
  return series;
}

//*创建并执行
inline void Workflow::start_series_work(SubTask *first, SubTask *last,
                                        series_callback_t callback) {
  SeriesWork *series = new SeriesWork(first, std::move(callback));
  series->set_last_task(last);
  first->dispatch();
}

//*并行任务流
class ParallelWork : public ParallelTask {
public:
  //*开启执行任务流
  void start() {
    assert(!series_of(this));
    Workflow::start_series_work(this, nullptr);
  }
  //*删除
  void dismiss() {
    assert(!series_of(this));
    delete this;
  }

public:
  //*添加串行任务
  void add_series(SeriesWork *series);

public:
  //*获取上下文
  void *get_context() const { return this->context; }
  //*设置上下文
  void set_context(void *context) { this->context = context; }

public:
  //*获取索引对应的串行任务流
  SeriesWork *series_at(size_t index) {
    if (index < this->subtasks_nr)
      return this->all_series[index];
    else
      return NULL;
  }
  //*获取索引对应的串行任务流
  const SeriesWork *series_at(size_t index) const {
    if (index < this->subtasks_nr)
      return this->all_series[index];
    else
      return NULL;
  }
  //*获取索引对应的串行任务流
  SeriesWork &operator[](size_t index) { return *this->series_at(index); }
  //*获取索引对应的串行任务流
  const SeriesWork &operator[](size_t index) const {
    return *this->series_at(index);
  }

  size_t size() const { return this->subtasks_nr; }

public:
  //*设置回调函数
  void set_callback(parallel_callback_t callback) {
    this->callback = std::move(callback);
  }

protected:
  virtual SubTask *done();

protected:
  void *context;                //*回调函数上下文
  parallel_callback_t callback; //*回调函数

private:
  //*扩容
  void expand_buf();

private:
  size_t buf_size;         //*缓冲区大小
  SeriesWork **all_series; //*串行任务队列

protected:
  ParallelWork(parallel_callback_t &&callback);
  ParallelWork(SeriesWork *const all_series[], size_t n,
               parallel_callback_t &&callback);
  virtual ~ParallelWork();
  friend class Workflow;
};

//*创建并型任务流但不添加任务
inline ParallelWork *
Workflow::create_parallel_work(parallel_callback_t callback) {
  return new ParallelWork(std::move(callback));
}

//*穿建并行任务流并添加任务
inline ParallelWork *
Workflow::create_parallel_work(SeriesWork *const all_series[], size_t n,
                               parallel_callback_t callback) {
  return new ParallelWork(all_series, n, std::move(callback));
}

//*开启执行并行任务流
inline void Workflow::start_parallel_work(SeriesWork *const all_series[],
                                          size_t n,
                                          parallel_callback_t callback) {
  ParallelWork *p = new ParallelWork(all_series, n, std::move(callback));
  Workflow::start_series_work(p, nullptr);
}
