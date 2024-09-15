#include <cstddef>  // 包含 size_t 的定义
#include <iostream>

struct PollerMessage {
    int (*append)(const void *, size_t *, PollerMessage *);
    char data[0];
};

int main() {
    std::cout << "Size of size_t: " << sizeof(size_t) << std::endl;
    return 0;
}
