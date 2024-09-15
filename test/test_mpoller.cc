/*
 * @Author       : muqiu0614 3155833132@qq.com
 * @Date         : 2024-09-12 16:23:17
 * @LastEditors  : muqiu0614 3155833132@qq.com
 * @LastEditTime : 2024-09-12 19:30:20
 * @FilePath     : /myworkflow/test/test_mpoller.cc
 * @Description  :
 * Copyright (c) 2024 by muqiu0614 email: 3155833132@qq.com, All Rights
 * Reserved.
 */
#include "../include/workflow.h"
#include <arpa/inet.h>
#include <iostream>
#include <string>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>
using namespace std;
class megqueue {
public:
  void push(poller_result *res) { vec.push_back(res); }
  vector<poller_result *> vec;
};

void callback(struct poller_result *res, void *temp) {
  megqueue *msg = (megqueue *)temp;
  msg->push(res);
}

int main() {
  megqueue *meg = new megqueue();
  poller_params *params = new poller_params;
  params->max_open_files = 100;
  params->callback = &callback;
  params->context = meg;
  mpoller_t *poller = mpoller_create(params, 3);

  poller_data *data = new poller_data;
  data->operation = PD_OP_READ;

}
