#include <iostream>
using namespace std;
struct test {
  int b;
  int *a[1];
};
int main() {
  test *a = (test *)malloc(offsetof(test, a) + 9 * sizeof(int *));
  for (int i = 0; i < 10; i++) {
    a->a[i] = new int(10);
  }

  for (int i = 0; i < 10; i++) {
    cout << *(a->a[i]) << endl;
  }
  cout << "sizeof  =  " << sizeof(*a) << endl;
}
