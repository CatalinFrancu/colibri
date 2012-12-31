#include <stdio.h>
#include "bitmanip.h"

int main() {
  printf("%llX\n", reverseBytes(0x1011121314151617));
  return 0;
}
