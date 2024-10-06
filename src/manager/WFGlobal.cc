/*
 * @Author       : gyy0727 3155833132@qq.com
 * @Date         : 2024-10-04 21:49:15
 * @LastEditors  : gyy0727 3155833132@qq.com
 * @LastEditTime : 2024-10-05 12:24:15
 * @FilePath     : /myworkflow/src/manager/WFGlobal.cc
 * @Description  :
 * Copyright (c) 2024 by gyy0727 email: 3155833132@qq.com, All Rights Reserved.
 */

#include "WFGlobal.h"
#include "../factory/WFTaskError.h"
#include "../kernel/CommScheduler.h"
#include "../kernel/Executor.h"
#include "../util/URIParser.h"
#include <arpa/inet.h>
#include <atomic>
#include <condition_variable>
#include <cstring>
#include <ctype.h>
#include <mutex>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <unordered_map>

class __WFGlobal {
public:
  static __WFGlobal *get_instance() {
    static __WFGlobal kInstance;
    return &kInstance;
  }

  const char *get_default_port(const std::string &scheme) {
    const auto it = static_scheme_port_.find(scheme);

    if (it != static_scheme_port_.end())
      return it->second;

    const char *port = NULL;
    user_scheme_port_mutex_.lock();
    const auto it2 = user_scheme_port_.find(scheme);

    if (it2 != user_scheme_port_.end())
      port = it2->second.c_str();

    user_scheme_port_mutex_.unlock();
    return port;
  }

  void register_scheme_port(const std::string &scheme, unsigned short port) {
    user_scheme_port_mutex_.lock();
    user_scheme_port_[scheme] = std::to_string(port);
    user_scheme_port_mutex_.unlock();
  }

  void sync_operation_begin() {
    bool inc;

    sync_mutex_.lock();
    inc = ++sync_count_ > sync_max_;
    if (inc)
      sync_max_ = sync_count_;

    sync_mutex_.unlock();
    if (inc)
      WFGlobal::increase_handler_thread();
  }

  void sync_operation_end() {
    int dec = 0;

    sync_mutex_.lock();
    if (--sync_count_ < (sync_max_ + 1) / 2) {
      dec = sync_max_ - 2 * sync_count_;
      sync_max_ -= dec;
    }

    sync_mutex_.unlock();
    while (dec > 0) {
      WFGlobal::decrease_handler_thread();
      dec--;
    }
  }

private:
  __WFGlobal();

private:
  std::unordered_map<std::string, const char *> static_scheme_port_;
  std::unordered_map<std::string, std::string> user_scheme_port_;
  std::mutex user_scheme_port_mutex_;
  std::mutex sync_mutex_;
  int sync_count_;
  int sync_max_;
};

__WFGlobal::__WFGlobal() {
  static_scheme_port_["dns"] = "53";
  static_scheme_port_["Dns"] = "53";
  static_scheme_port_["DNS"] = "53";

  static_scheme_port_["http"] = "80";
  static_scheme_port_["Http"] = "80";
  static_scheme_port_["HTTP"] = "80";

  static_scheme_port_["redis"] = "6379";
  static_scheme_port_["Redis"] = "6379";
  static_scheme_port_["REDIS"] = "6379";

  static_scheme_port_["mysql"] = "3306";
  static_scheme_port_["Mysql"] = "3306";
  static_scheme_port_["MySql"] = "3306";
  static_scheme_port_["MySQL"] = "3306";
  static_scheme_port_["MYSQL"] = "3306";

  static_scheme_port_["kafka"] = "9092";
  static_scheme_port_["Kafka"] = "9092";
  static_scheme_port_["KAFKA"] = "9092";

  sync_count_ = 0;
  sync_max_ = 0;
}

class __FileIOService : public IOService {
public:
  __FileIOService(CommScheduler *scheduler)
      : scheduler_(scheduler), flag_(true) {}

  int bind() {
    mutex_.lock();
    flag_ = false;

    int ret = scheduler_->io_bind(this);

    if (ret < 0)
      flag_ = true;

    mutex_.unlock();
    return ret;
  }

  void deinit() {
    std::unique_lock<std::mutex> lock(mutex_);
    while (!flag_)
      cond_.wait(lock);

    lock.unlock();
    IOService::deinit();
  }

private:
  virtual void handle_unbound() {
    mutex_.lock();
    flag_ = true;
    cond_.notify_one();
    mutex_.unlock();
  }

  virtual void handle_stop(int error) { scheduler_->io_unbind(this); }

  CommScheduler *scheduler_;
  std::mutex mutex_;
  std::condition_variable cond_;
  bool flag_;
};

class __ThreadDnsManager {
public:
  static __ThreadDnsManager *get_instance() {
    static __ThreadDnsManager kInstance;
    return &kInstance;
  }

  ExecQueue *get_dns_queue() { return &dns_queue_; }
  Executor *get_dns_executor() { return &dns_executor_; }

  __ThreadDnsManager() {
    int ret;

    ret = dns_queue_.init();
    if (ret < 0)
      abort();

    ret = dns_executor_.init(WFGlobal::get_global_settings()->dns_threads);
    if (ret < 0)
      abort();
  }

  ~__ThreadDnsManager() {
    dns_executor_.deinit();
    dns_queue_.deinit();
  }

private:
  ExecQueue dns_queue_;
  Executor dns_executor_;
};

class __CommManager {
public:
  static __CommManager *get_instance() {
    static __CommManager kInstance;
    __CommManager::created_ = true;
    return &kInstance;
  }

  CommScheduler *get_scheduler() { return &scheduler_; }
  IOService *get_io_service();
  static bool is_created() { return created_; }

private:
  __CommManager() : fio_service_(NULL), fio_flag_(false) {
    const auto *settings = WFGlobal::get_global_settings();
    if (scheduler_.init(settings->poller_threads, settings->handler_threads) <
        0)
      abort();

    signal(SIGPIPE, SIG_IGN);
  }

  ~__CommManager() {
    // scheduler_.deinit() will triger fio_service to stop
    scheduler_.deinit();
    if (fio_service_) {
      fio_service_->deinit();
      delete fio_service_;
    }
  }

private:
  CommScheduler scheduler_;
  __FileIOService *fio_service_;
  volatile bool fio_flag_;
  std::mutex fio_mutex_;

private:
  static bool created_;
};

bool __CommManager::created_ = false;

inline IOService *__CommManager::get_io_service() {
  if (!fio_flag_) {
    fio_mutex_.lock();
    if (!fio_flag_) {
      int maxevents = WFGlobal::get_global_settings()->fio_max_events;

      fio_service_ = new __FileIOService(&scheduler_);
      if (fio_service_->init(maxevents) < 0)
        abort();

      if (fio_service_->bind() < 0)
        abort();

      fio_flag_ = true;
    }

    fio_mutex_.unlock();
  }

  return fio_service_;
}

class __ExecManager {
protected:
  using ExecQueueMap = std::unordered_map<std::string, ExecQueue *>;

public:
  static __ExecManager *get_instance() {
    static __ExecManager kInstance;
    return &kInstance;
  }

  ExecQueue *get_exec_queue(const std::string &queue_name);
  Executor *get_compute_executor() { return &compute_executor_; }

private:
  __ExecManager() : rwlock_(PTHREAD_RWLOCK_INITIALIZER) {
    int compute_threads = WFGlobal::get_global_settings()->compute_threads;

    if (compute_threads < 0)
      compute_threads = sysconf(_SC_NPROCESSORS_ONLN);

    if (compute_executor_.init(compute_threads) < 0)
      abort();
  }

  ~__ExecManager() {
    compute_executor_.deinit();

    for (auto &kv : queue_map_) {
      kv.second->deinit();
      delete kv.second;
    }
  }

private:
  pthread_rwlock_t rwlock_;
  ExecQueueMap queue_map_;
  Executor compute_executor_;
};

inline ExecQueue *__ExecManager::get_exec_queue(const std::string &queue_name) {
  ExecQueue *queue = NULL;
  ExecQueueMap::const_iterator iter;

  pthread_rwlock_rdlock(&rwlock_);
  iter = queue_map_.find(queue_name);
  if (iter != queue_map_.cend())
    queue = iter->second;

  pthread_rwlock_unlock(&rwlock_);
  if (queue)
    return queue;

  pthread_rwlock_wrlock(&rwlock_);
  iter = queue_map_.find(queue_name);
  if (iter == queue_map_.cend()) {
    queue = new ExecQueue();
    if (queue->init() >= 0)
      queue_map_.emplace(queue_name, queue);
    else {
      delete queue;
      queue = NULL;
    }
  } else
    queue = iter->second;

  pthread_rwlock_unlock(&rwlock_);
  return queue;
}

static inline const char *__try_options(const char *p, const char *q,
                                        const char *r) {
  size_t len = strlen(r);
  if ((size_t)(q - p) >= len && strncmp(p, r, len) == 0)
    return p + len;
  return NULL;
}

static void __set_options(const char *p, int *ndots, int *attempts,
                          bool *rotate) {
  const char *start;
  const char *opt;

  if (!isspace(*p))
    return;

  while (1) {
    while (isspace(*p))
      p++;

    start = p;
    while (*p && *p != '#' && *p != ';' && !isspace(*p))
      p++;

    if (start == p)
      break;

    if ((opt = __try_options(start, p, "ndots:")) != NULL)
      *ndots = atoi(opt);
    else if ((opt = __try_options(start, p, "attempts:")) != NULL)
      *attempts = atoi(opt);
    else if ((opt = __try_options(start, p, "rotate")) != NULL)
      *rotate = true;
  }
}

struct WFGlobalSettings WFGlobal::settings_ = GLOBAL_SETTINGS_DEFAULT;
bool WFGlobal::is_scheduler_created() { return __CommManager::is_created(); }

CommScheduler *WFGlobal::get_scheduler() {
  return __CommManager::get_instance()->get_scheduler();
}

ExecQueue *WFGlobal::get_exec_queue(const std::string &queue_name) {
  return __ExecManager::get_instance()->get_exec_queue(queue_name);
}

Executor *WFGlobal::get_compute_executor() {
  return __ExecManager::get_instance()->get_compute_executor();
}

IOService *WFGlobal::get_io_service() {
  return __CommManager::get_instance()->get_io_service();
}

ExecQueue *WFGlobal::get_dns_queue() {
  return __ThreadDnsManager::get_instance()->get_dns_queue();
}

Executor *WFGlobal::get_dns_executor() {
  return __ThreadDnsManager::get_instance()->get_dns_executor();
}

const char *WFGlobal::get_default_port(const std::string &scheme) {
  return __WFGlobal::get_instance()->get_default_port(scheme);
}

void WFGlobal::register_scheme_port(const std::string &scheme,
                                    unsigned short port) {
  __WFGlobal::get_instance()->register_scheme_port(scheme, port);
}

int WFGlobal::sync_operation_begin() {
  if (WFGlobal::is_scheduler_created() &&
      WFGlobal::get_scheduler()->is_handler_thread()) {
    __WFGlobal::get_instance()->sync_operation_begin();
    return 1;
  }

  return 0;
}

void WFGlobal::sync_operation_end(int cookie) {
  if (cookie)
    __WFGlobal::get_instance()->sync_operation_end();
}

static inline const char *__get_task_error_string(int error) {
  switch (error) {
  case WFT_ERR_URI_PARSE_FAILED:
    return "URI Parse Failed";

  case WFT_ERR_URI_SCHEME_INVALID:
    return "URI Scheme Invalid";

  case WFT_ERR_URI_PORT_INVALID:
    return "URI Port Invalid";

  case WFT_ERR_UPSTREAM_UNAVAILABLE:
    return "Upstream Unavailable";

  case WFT_ERR_HTTP_BAD_REDIRECT_HEADER:
    return "Http Bad Redirect Header";

  case WFT_ERR_HTTP_PROXY_CONNECT_FAILED:
    return "Http Proxy Connect Failed";

  case WFT_ERR_REDIS_ACCESS_DENIED:
    return "Redis Access Denied";

  case WFT_ERR_REDIS_COMMAND_DISALLOWED:
    return "Redis Command Disallowed";

  case WFT_ERR_MYSQL_HOST_NOT_ALLOWED:
    return "MySQL Host Not Allowed";

  case WFT_ERR_MYSQL_ACCESS_DENIED:
    return "MySQL Access Denied";

  case WFT_ERR_MYSQL_INVALID_CHARACTER_SET:
    return "MySQL Invalid Character Set";

  case WFT_ERR_MYSQL_COMMAND_DISALLOWED:
    return "MySQL Command Disallowed";

  case WFT_ERR_MYSQL_QUERY_NOT_SET:
    return "MySQL Query Not Set";

  case WFT_ERR_MYSQL_SSL_NOT_SUPPORTED:
    return "MySQL SSL Not Supported";

  case WFT_ERR_KAFKA_PARSE_RESPONSE_FAILED:
    return "Kafka parse response failed";

  case WFT_ERR_KAFKA_PRODUCE_FAILED:
    return "Kafka produce api failed";

  case WFT_ERR_KAFKA_FETCH_FAILED:
    return "Kafka fetch api failed";

  case WFT_ERR_KAFKA_CGROUP_FAILED:
    return "Kafka cgroup failed";

  case WFT_ERR_KAFKA_COMMIT_FAILED:
    return "Kafka commit api failed";

  case WFT_ERR_KAFKA_META_FAILED:
    return "Kafka meta api failed";

  case WFT_ERR_KAFKA_LEAVEGROUP_FAILED:
    return "Kafka leavegroup failed";

  case WFT_ERR_KAFKA_API_UNKNOWN:
    return "Kafka api type unknown";

  case WFT_ERR_KAFKA_VERSION_DISALLOWED:
    return "Kafka broker version not supported";

  case WFT_ERR_KAFKA_SASL_DISALLOWED:
    return "Kafka sasl disallowed";

  case WFT_ERR_KAFKA_ARRANGE_FAILED:
    return "Kafka arrange failed";

  case WFT_ERR_KAFKA_LIST_OFFSETS_FAILED:
    return "Kafka list offsets failed";

  case WFT_ERR_KAFKA_CGROUP_ASSIGN_FAILED:
    return "Kafka cgroup assign failed";

  case WFT_ERR_CONSUL_API_UNKNOWN:
    return "Consul api type unknown";

  case WFT_ERR_CONSUL_CHECK_RESPONSE_FAILED:
    return "Consul check response failed";

  default:
    break;
  }

  return "Unknown";
}

const char *WFGlobal::get_error_string(int state, int error) {
  switch (state) {
  case WFT_STATE_SUCCESS:
    return "Success";

  case WFT_STATE_TOREPLY:
    return "To Reply";

  case WFT_STATE_NOREPLY:
    return "No Reply";

  case WFT_STATE_SYS_ERROR:
    return strerror(error);

  case WFT_STATE_TASK_ERROR:
    return __get_task_error_string(error);

  case WFT_STATE_ABORTED:
    return "Aborted";

  case WFT_STATE_UNDEFINED:
    return "Undefined";

  default:
    break;
  }

  return "Unknown";
}

void WORKFLOW_library_init(const struct WFGlobalSettings *settings) {
  WFGlobal::set_global_settings(settings);
}
