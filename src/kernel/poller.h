/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-09-09 21:43:16
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-09-25 12:56:30
 * @FilePath     : /myworkflow/src/kernel/poller.h
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */

#ifndef _POLLER_H_
#define _POLLER_H_

#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>

typedef struct __poller poller_t;
typedef struct __poller_message poller_message_t;

struct __poller_message {
  int (*append)(const void *, size_t *,
                poller_message_t *); //*读取到消息的函数指针
  char data[0];                      //*缓冲区
};

struct poller_data {
#define PD_OP_TIMER 0    //*超时事件
#define PD_OP_READ 1     //*读事件
#define PD_OP_WRITE 2    //*写事件
#define PD_OP_LISTEN 3   //*监听事件
#define PD_OP_CONNECT 4  //*连接事件
#define PD_OP_RECVFROM 5 //*recvfrom事件
#define PD_OP_EVENT 9    //*事件
#define PD_OP_NOTIFY 10  //*通知事件
  short operation;       //*要执行的操作
  unsigned short iovcnt; //*iovc结构体数量
  int fd;                //*对应的文件描述符
  union {
    poller_message_t *(*create_message)(void *);
    int (*partial_written)(size_t, void *);
    void *(*accept)(const struct sockaddr *, socklen_t, int, void *);
    void *(*recvfrom)(const struct sockaddr *, socklen_t, const void *, size_t,
                      void *);
    void *(*event)(void *);
    void *(*notify)(void *, void *);
  };
  void *context; //*回调函数上下文
  union {
    poller_message_t *message;
    struct iovec *write_iov;
    void *result;
  };
};

struct poller_result {
#define PR_ST_SUCCESS 0    //*成功
#define PR_ST_FINISHED 1   //*结束
#define PR_ST_ERROR 2      //*出错
#define PR_ST_DELETED 3    //*删除
#define PR_ST_MODIFIED 4   //*修改
#define PR_ST_STOPPED 5    //*暂停
  int state;               //*处理状态
  int error;               //*错误码
  struct poller_data data; //*回调结果结构体
};

//*poller的参数
struct poller_params {
  size_t max_open_files;
  void (*callback)(struct poller_result *, void *); //*回调函数
  void *context;                                    //*上下文
};

#ifdef __cplusplus
extern "C" {
#endif

poller_t *poller_create(const struct poller_params *params);
int poller_start(poller_t *poller);
int poller_add(const struct poller_data *data, int timeout, poller_t *poller);
int poller_del(int fd, poller_t *poller);
int poller_mod(const struct poller_data *data, int timeout, poller_t *poller);
int poller_set_timeout(int fd, int timeout, poller_t *poller);
int poller_add_timer(const struct timespec *value, void *context, void **timer,
                     poller_t *poller);
int poller_del_timer(void *timer, poller_t *poller);
void poller_stop(poller_t *poller);
void poller_destroy(poller_t *poller);

#ifdef __cplusplus
}
#endif

#endif
