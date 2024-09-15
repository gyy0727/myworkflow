/*
 * @Author       : muqiu0614 3155833132@qq.com
 * @Date         : 2024-09-12 17:10:33
 * @LastEditors  : muqiu0614 3155833132@qq.com
 * @LastEditTime : 2024-09-12 18:49:01
 * @FilePath     : /myworkflow/test/test_httpparser.cc
 * @Description  :
 * Copyright (c) 2024 by muqiu0614 email: 3155833132@qq.com, All Rights
 * Reserved.
 */
#include "../src/protocol/http_parser.h"
#include <cstdlib>
#include <spdlog/spdlog.h>
#include <stdio.h>
#include <string.h>

const char *http_request_data =
    "GET /index.html HTTP/1.1\r\n"
    "Host: www.example.com\r\n"
    "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n"
    "Accept: "
    "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n"
    "Accept-Language: en-US,en;q=0.5\r\n"
    "Accept-Encoding: gzip, deflate\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "This is body.";

int main() {
  http_parser_t parser;
  http_header_cursor_t cursor;
  size_t data_len = strlen(http_request_data);
  size_t offset = 0;
  const void *name, *value;
  size_t name_len, value_len;
  const void *body;
  size_t body_size;

  // 初始化解析器
  http_parser_init(0, &parser);

  // 循环解析数据
  while (offset < data_len) {
    int ret = http_parser_append_message(&http_request_data[offset], &data_len,
                                         &parser);
    if (ret < 0) {
      fprintf(stderr, "Error parsing HTTP request.\n");
      exit(EXIT_FAILURE);
    }
    offset += data_len;
  }

  // 获取解析后的信息
  printf("Method: %s\n", http_parser_get_method(&parser));
  printf("URI: %s\n", http_parser_get_uri(&parser));
  printf("Version: %s\n", http_parser_get_version(&parser));
  //  printf("Body: %s\n", http_parser_get_body(&parser));
  // 初始化游标
  http_header_cursor_init(&cursor, &parser);

  // 查找并打印特定头部字段
  if (http_header_cursor_find((const void *)"User-Agent", strlen("User-Agent"),
                              &value, &value_len, &cursor) == 0) {
    printf("User-Agent: %.*s\n", (int)value_len, (const char *)value);
  } else {
    printf("User-Agent header not found.\n");
  }

  if (http_header_cursor_find((const void *)"Accept", strlen("Accept"), &value,
                              &value_len, &cursor) == 0) {
    printf("Accept: %.*s\n", (int)value_len, (const char *)value);
  } else {
    printf("Accept header not found.\n");
  }

  if (http_header_cursor_find((const void *)"Accept-Language",
                              strlen("Accept-Language"), &value, &value_len,
                              &cursor) == 0) {
    printf("Accept-Language: %.*s\n", (int)value_len, (const char *)value);
  } else {
    printf("Accept-Language header not found.\n");
  }

  if (http_header_cursor_find((const void *)"Accept-Encoding",
                              strlen("Accept-Encoding"), &value, &value_len,
                              &cursor) == 0) {
    printf("Accept-Encoding: %.*s\n", (int)value_len, (const char *)value);
  } else {
    printf("Accept-Encoding header not found.\n");
  }

  if (http_header_cursor_find((const void *)"Connection", strlen("Connection"),
                              &value, &value_len, &cursor) == 0) {
    printf("Connection: %.*s\n", (int)value_len, (const char *)value);
  } else {
    printf("Connection header not found.\n");
  }

  // 检查是否有Body部分
  if (http_parser_get_body(&body, &body_size, &parser) == 0) {
    printf("Body: ");
    for (size_t i = 0; i < body_size; ++i) {
      printf("%c", ((char *)body)[i]);
    }
    printf("\n");
  }

  // 清理
  http_parser_deinit(&parser);

  return 0;
}