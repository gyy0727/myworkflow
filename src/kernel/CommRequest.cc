/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-09 21:43:16
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-05 18:18:51
 * @FilePath     : /myworkflow/src/kernel/CommRequest.cc
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

  Author: Xie Han (xiehan@sogou-inc.com)
*/

#include "CommRequest.h"
#include "CommScheduler.h"
#include <errno.h>

void CommRequest::handle(int state, int error) {
  std::cout << " CommRequest::handle" << std::endl;
  this->state = state;
  this->error = error;
  if (error != ETIMEDOUT)
    this->timeout_reason = TOR_NOT_TIMEOUT;
  else if (!this->target)
    this->timeout_reason = TOR_WAIT_TIMEOUT;
  else if (!this->get_message_out())
    this->timeout_reason = TOR_CONNECT_TIMEOUT;
  else
    this->timeout_reason = TOR_TRANSMIT_TIMEOUT;

  this->subtask_done();
}
