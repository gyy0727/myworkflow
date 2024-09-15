#include <iostream>
#include <spdlog/spdlog.h>
#include <thread>
#include <unistd.h>
using namespace std;

void printf_() {
  for (int i = 0; i < 100; i++) {
    spdlog::info(" print : {}", i);
  }
}
int main() {
  thread t1(printf_);

  thread t2(printf_);

  thread t3(printf_);

  sleep(10);
  return 0;
}
