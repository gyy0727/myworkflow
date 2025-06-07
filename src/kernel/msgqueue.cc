
/*
 * This message queue originates from the project of Sogou C++ Workflow:
 * https://github.com/sogou/workflow
 *
 * The idea of this implementation is quite simple and obvious. When the
 * get_list is not empty, the consumer takes a message. Otherwise the consumer
 * waits till put_list is not empty, and swap two lists. This method performs
 * well when the queue is very busy, and the number of consumers is big.
 */

#include "msgqueue.h"
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

struct __msgqueue {
  size_t msg_max;            //*最大的消息数量
  size_t msg_cnt;            //*当前消息数量
  int linkoff;               //*代表消息的大小
  int nonblock;              //*是否阻塞
  void *head1;               //*缓冲区1
  void *head2;               //*缓冲区2
  void **get_head;           //*获取头部
  void **put_head;           //*存放头部
  void **put_tail;           //*存放尾部
  pthread_mutex_t get_mutex; //*获取锁
  pthread_mutex_t put_mutex; //*存放锁
  pthread_cond_t get_cond;   //*获取信号量
  pthread_cond_t put_cond;   //*存放信号量
};

//*设置消息队列为非阻塞
void msgqueue_set_nonblock(msgqueue_t *queue) {
  queue->nonblock = 1;
  pthread_mutex_lock(&queue->put_mutex);
  pthread_cond_signal(&queue->get_cond);    //*唤醒一个在等待的线程
  pthread_cond_broadcast(&queue->put_cond); //*唤醒所有在等待的线程
  pthread_mutex_unlock(&queue->put_mutex);
}

//*将消息队列设置为阻塞
void msgqueue_set_block(msgqueue_t *queue) { queue->nonblock = 0; }

//*存入尾部
void msgqueue_put(void *msg, msgqueue_t *queue) {
  
  void **link = (void **)((char *)msg + queue->linkoff);
  *link = NULL;
  pthread_mutex_lock(&queue->put_mutex);
  while (queue->msg_cnt > queue->msg_max - 1 && !queue->nonblock)
    pthread_cond_wait(&queue->put_cond, &queue->put_mutex);

  *queue->put_tail = link;
  queue->put_tail = link;
  queue->msg_cnt++;
  pthread_mutex_unlock(&queue->put_mutex);
  pthread_cond_signal(&queue->get_cond);
}

//*放到首部
void msgqueue_put_head(void *msg, msgqueue_t *queue) {
  void **link = (void **)((char *)msg + queue->linkoff);

  pthread_mutex_lock(&queue->put_mutex);
  while (*queue->get_head) {
    if (pthread_mutex_trylock(&queue->get_mutex) == 0) {
      pthread_mutex_unlock(&queue->put_mutex);
      *link = *queue->get_head;
      *queue->get_head = link;
      pthread_mutex_unlock(&queue->get_mutex);
      return;
    }
  }

  while (queue->msg_cnt > queue->msg_max - 1 && !queue->nonblock)
    pthread_cond_wait(&queue->put_cond, &queue->put_mutex);

  *link = *queue->put_head;
  if (*link == NULL)
    queue->put_tail = link;

  *queue->put_head = link;
  queue->msg_cnt++;
  pthread_mutex_unlock(&queue->put_mutex);
  pthread_cond_signal(&queue->get_cond);
}

//*交换两个缓冲区
static size_t __msgqueue_swap(msgqueue_t *queue) {
  void **get_head = queue->get_head;
  size_t cnt;

  pthread_mutex_lock(&queue->put_mutex);
  while (queue->msg_cnt == 0 && !queue->nonblock)
    pthread_cond_wait(&queue->get_cond, &queue->put_mutex);

  cnt = queue->msg_cnt;
  if (cnt > queue->msg_max - 1)
    pthread_cond_broadcast(&queue->put_cond);

  queue->get_head = queue->put_head;
  queue->put_head = get_head;
  queue->put_tail = get_head;
  queue->msg_cnt = 0;
  pthread_mutex_unlock(&queue->put_mutex);
  return cnt;
}

//*获取数据
void *msgqueue_get(msgqueue_t *queue) {
  void *msg;

  pthread_mutex_lock(&queue->get_mutex);
  if (*queue->get_head || __msgqueue_swap(queue) > 0) {
    msg = (char *)*queue->get_head - queue->linkoff;
    *queue->get_head = *(void **)*queue->get_head;
  } else
    msg = NULL;

  pthread_mutex_unlock(&queue->get_mutex);
  return msg;
}

//*创建消息队列
msgqueue_t *msgqueue_create(size_t maxlen, int linkoff) {
  msgqueue_t *queue = (msgqueue_t *)malloc(sizeof(msgqueue_t));
  int ret;

  if (!queue)
    return NULL;

  ret = pthread_mutex_init(&queue->get_mutex, NULL);
  if (ret == 0) {
    ret = pthread_mutex_init(&queue->put_mutex, NULL);
    if (ret == 0) {
      ret = pthread_cond_init(&queue->get_cond, NULL);
      if (ret == 0) {
        ret = pthread_cond_init(&queue->put_cond, NULL);
        if (ret == 0) {
          queue->msg_max = maxlen;
          queue->linkoff = linkoff; //*偏移量
          queue->head1 = NULL;
          queue->head2 = NULL;
          queue->get_head = &queue->head1;
          queue->put_head = &queue->head2;
          queue->put_tail = &queue->head2;
          queue->msg_cnt = 0;
          queue->nonblock = 0;
          return queue;
        }

        pthread_cond_destroy(&queue->get_cond);
      }

      pthread_mutex_destroy(&queue->put_mutex);
    }

    pthread_mutex_destroy(&queue->get_mutex);
  }

  errno = ret;
  free(queue);
  return NULL;
}

//*释放队列
void msgqueue_destroy(msgqueue_t *queue) {
  pthread_cond_destroy(&queue->put_cond);
  pthread_cond_destroy(&queue->get_cond);
  pthread_mutex_destroy(&queue->put_mutex);
  pthread_mutex_destroy(&queue->get_mutex);
  free(queue);
}
