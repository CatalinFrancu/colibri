#include <stdio.h>
#include "egtb.h"
#include "precomp.h"

int main() {
  precomputeAll();
  generateEgtb("KvR");
  return 0;
}
