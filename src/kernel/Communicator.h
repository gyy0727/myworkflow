#ifndef _COMMUNICATOR_H_
#define _COMMUNICATOR_H_

#include "list.h"
#include "poller.h"
#include <iostream>
#include <pthread.h>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
using namespace std;
//*连接实例
class CommConnection {
public:
  virtual ~CommConnection() {}
};

//*连接目标实例
class CommTarget {
public:
  int init(const struct sockaddr *addr, socklen_t addrlen, int connect_timeout,
           int response_timeout);
  void deinit();

public:
  //*addr=this->addr
  void get_addr(const struct sockaddr **addr, socklen_t *addrlen) const {
    *addr = this->addr;
    *addrlen = this->addrlen;
  }
  //*是否有空闲链接,检查idle_list是否empty
  int has_idle_conn() const { return !list_empty(&this->idle_list); }

private:
  //*根据this->addr->sa_family创建socket文件描述符
  virtual int create_connect_fd() {
    return socket(this->addr->sa_family, SOCK_STREAM, 0);
  }

  //*创建新连接,用于客户端
  virtual CommConnection *new_connection(int connect_fd) {
    return new CommConnection;
  }

public:
  //*释放
  virtual void release() {}

private:
  struct sockaddr *addr; //*要连接的目标地址
  socklen_t addrlen;     //*IP地址长度
  int connect_timeout;   //*连接超时
  int response_timeout;  //*响应超时

private:
  struct list_head idle_list; //*空闲列表链表
  pthread_mutex_t mutex;

public:
  virtual ~CommTarget() {}
  friend class CommServiceTarget;
  friend class Communicator;
};

//*要向连接上发送的数据
class CommMessageOut {
private:
  //*序列化
  virtual int encode(struct iovec vectors[], int max) = 0;

public:
  virtual ~CommMessageOut() {}
  friend class Communicator;
};

//*连接上接收到的数据
class CommMessageIn : private poller_message_t {
private:
  //*将从socket文件描述符读取到的数据添加到本结构体内
  virtual int append(const void *buf, size_t *size) = 0;

protected:
  virtual int feedback(const void *buf, size_t size);

  virtual void renew();

  virtual CommMessageIn *inner() { return this; }

private:
  struct CommConnEntry *entry;

public:
  virtual ~CommMessageIn() {}
  friend class Communicator;
};

#define CS_STATE_SUCCESS 0
#define CS_STATE_ERROR 1
#define CS_STATE_STOPPED 2
#define CS_STATE_TOREPLY 3 /* for service session only. */

//*一次会话,其实就是一次task
class CommSession {
private:
  virtual CommMessageOut *message_out() = 0;
  virtual CommMessageIn *message_in() = 0;
  virtual int send_timeout() { return -1; }
  virtual int receive_timeout() { return -1; }
  virtual int keep_alive_timeout() { return 0; }
  virtual int first_timeout() { return 0; }
  virtual void handle(int state, int error) = 0; //*处理执行结果

protected:
  CommTarget *get_target() const { return this->target; }
  CommConnection *get_connection() const { return this->conn; }
  CommMessageOut *get_message_out() const { return this->out; }
  CommMessageIn *get_message_in() const { return this->in; }
  long long get_seq() const { return this->seq; } //*获取序列号

private:
  CommTarget *target;   //*连接的目标
  CommConnection *conn; //*连接实例
  CommMessageOut *out;  //*发送数据
  CommMessageIn *in;    //*接受数据
  long long seq;        //*序列号

private:
  struct timespec begin_time; //*会话开始时间
  int timeout;                //*超时时间
  int passive;
  //*如果 passive 设置为 1 或true，
  //*则表示该会话处于被动模式。在这种模式下，会话可能不会主动发起某些操作，而是等待另一端的请求或响应。
  //*例如，在某些网络协议中，客户端通常处于主动模式，而服务器端则处于被动模式，等待客户端连接。
public:
  CommSession() { this->passive = 0; }
  virtual ~CommSession();
  friend class CommMessageIn;
  friend class Communicator;
};

//*相当于一个监听服务
class CommService {
public:
  //*初始化
  int init(const struct sockaddr *bind_addr, socklen_t addrlen,
           int listen_timeout, int response_timeout);
  //*重置
  void deinit();
  //*减少连接
  int drain(int max);

public:
  //*获取绑定的地址
  void get_addr(const struct sockaddr **addr, socklen_t *addrlen) const {
    *addr = this->bind_addr;
    *addrlen = this->addrlen;
  }

private:
  //*新建会话
  virtual CommSession *new_session(long long seq, CommConnection *conn) = 0;
  //*停止监听
  virtual void handle_stop(int error) {}
  //*解除绑定
  virtual void handle_unbound() = 0;

private:
  //*创建监听描述符
  virtual int create_listen_fd() {
    return socket(this->bind_addr->sa_family, SOCK_STREAM, 0);
  }

  virtual CommConnection *new_connection(int accept_fd) {
    return new CommConnection;
  }

private:
  struct sockaddr *bind_addr; //*绑定的本地地址
  socklen_t addrlen;          //*地址长度
  int listen_timeout;         //*监听超时
  int response_timeout;       //*响应超时

private:
  void incref();
  void decref();

private:
  int reliable;//*可靠通信,即tcp和udp
  //*是否成功listen
  //*if (ret >= 0 || errno == EOPNOTSUPP)
  //*{
  //*service->reliable = (ret >= 0);
  //*return sockfd;
  //*}
  int listen_fd; //*监听描述符
  int ref;       //*引用计数

private:
  struct list_head alive_list; //*活跃的连接
  pthread_mutex_t mutex;

public:
  virtual ~CommService() {}
  friend class CommServiceTarget;
  friend class Communicator;
};

#define SS_STATE_COMPLETE 0
#define SS_STATE_ERROR 1
#define SS_STATE_DISRUPTED 2

class SleepSession {
private:
  virtual int duration(struct timespec *value) = 0;
  virtual void handle(int state, int error) = 0;

private:
  void *timer;
  int index;

public:
  virtual ~SleepSession() {}
  friend class Communicator;
};

#ifdef __linux__
#include "IOService_linux.h"
#else
#include "IOService_thread.h"
#endif

class Communicator {
public:
  int init(size_t poller_threads, size_t handler_threads);
  void deinit();

  int request(CommSession *session, CommTarget *target);
  int reply(CommSession *session);

  int push(const void *buf, size_t size, CommSession *session);

  int shutdown(CommSession *session);

  int bind(CommService *service);
  void unbind(CommService *service);

  int sleep(SleepSession *session);
  int unsleep(SleepSession *session);

  int io_bind(IOService *service);
  void io_unbind(IOService *service);

public:
  int is_handler_thread() const;

  int increase_handler_thread();
  int decrease_handler_thread();

private:
  struct __mpoller *mpoller;
  struct __msgqueue *msgqueue;
  struct __thrdpool *thrdpool;
  int stop_flag;

private:
  int create_poller(size_t poller_threads);

  int create_handler_threads(size_t handler_threads);

  void shutdown_service(CommService *service);

  void shutdown_io_service(IOService *service);

  int send_message_sync(struct iovec vectors[], int cnt,
                        struct CommConnEntry *entry);
  int send_message_async(struct iovec vectors[], int cnt,
                         struct CommConnEntry *entry);

  int send_message(struct CommConnEntry *entry);

  int request_new_conn(CommSession *session, CommTarget *target);
  int request_idle_conn(CommSession *session, CommTarget *target);

  int reply_message_unreliable(struct CommConnEntry *entry);

  int reply_reliable(CommSession *session, CommTarget *target);
  int reply_unreliable(CommSession *session, CommTarget *target);

  void handle_incoming_request(struct poller_result *res);
  void handle_incoming_reply(struct poller_result *res);

  void handle_request_result(struct poller_result *res);
  void handle_reply_result(struct poller_result *res);

  void handle_write_result(struct poller_result *res);
  void handle_read_result(struct poller_result *res);

  void handle_connect_result(struct poller_result *res);
  void handle_listen_result(struct poller_result *res);

  void handle_recvfrom_result(struct poller_result *res);

  void handle_sleep_result(struct poller_result *res);

  void handle_aio_result(struct poller_result *res);

  static void handler_thread_routine(void *context);

  static int nonblock_connect(CommTarget *target);
  static int nonblock_listen(CommService *service);

  static struct CommConnEntry *launch_conn(CommSession *session,
                                           CommTarget *target);
  static struct CommConnEntry *accept_conn(class CommServiceTarget *target,
                                           CommService *service);

  static int first_timeout(CommSession *session);
  static int next_timeout(CommSession *session);

  static int first_timeout_send(CommSession *session);
  static int first_timeout_recv(CommSession *session);

  static int append_message(const void *buf, size_t *size,
                            poller_message_t *msg);

  static poller_message_t *create_request(void *context);
  static poller_message_t *create_reply(void *context);

  static int recv_request(const void *buf, size_t size,
                          struct CommConnEntry *entry);

  static int partial_written(size_t n, void *context);

  static void *accept(const struct sockaddr *addr, socklen_t addrlen,
                      int sockfd, void *context);

  static void *recvfrom(const struct sockaddr *addr, socklen_t addrlen,
                        const void *buf, size_t size, void *context);

  static void callback(struct poller_result *res, void *context);

public:
  virtual ~Communicator() {}
};

#endif
