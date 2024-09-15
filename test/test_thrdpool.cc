/*
 * @Author       : muqiu0614 3155833132@qq.com
 * @Date         : 2024-09-10 18:29:46
 * @LastEditors  : muqiu0614 3155833132@qq.com
 * @LastEditTime : 2024-09-11 09:55:30
 * @FilePath     : /myworkflow/test/test_thrdpool.cc
 * @Description  :
 * Copyright (c) 2024 by muqiu0614 email: 3155833132@qq.com, All Rights
 * Reserved.
 */

#include "../include/workflow.h"
#include <cstdlib>
#include <iostream>
#include <thread>
#include <unistd.h>
using namespace std;
void print_(void *a) {
  for (int i = 0; i < 1000; i++) {
    cout << "i = " << this_thread::get_id() << endl;
    // sleep(1);
  }
}
int main() {
  thrdpool_t *thrdpool = thrdpool_create(3, 128 * 1024 * 1024);
  thrdpool_task *task = (thrdpool_task *)malloc(sizeof(thrdpool_task));
  task->routine = &print_;
  task->context = nullptr;
  thrdpool_schedule(task, thrdpool);
  thrdpool_schedule(task, thrdpool);
  thrdpool_schedule(task, thrdpool);
  sleep(10);
  thrdpool_destroy(nullptr, thrdpool);
}