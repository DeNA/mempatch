/*
 * Copyright 2024 DeNA Co., Ltd.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
 * either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <unistd.h>
#include <stdio.h>
#include <time.h>
#include <vector>

struct Random {
  unsigned int x;
  unsigned int y;
  unsigned int z;
  unsigned int w;
  Random() : x(0x34fb2383), y(0x327328fa), z(0xabd4b54a), w(0xa9dba8d1) {;}
  Random(int s) : x(0x34fb2383), y(0x327328fa), z(0xabd4b54a), w(s) {
    for (int i = 0; i < 100; i++) { Xor128(); }
  }
  void Seed(int s) {
    *this = Random(s);
  }
  unsigned int Xor128() {
    unsigned int t;
    t = x ^ (x << 11);
    x = y; y = z; z = w;
    return w = (w ^ (w >> 19)) ^ (t ^ (t >> 8));
  }
  int next(int r) { return Xor128() % r; }
  int next(int l, int r) { return next(r - l + 1) + l; }
};
Random rnd;

int main() {
  const int SIZE1 = 1024 * 100;
  const int SIZE2 = 1024;
  Random rnd(time(nullptr));
  std::vector<std::vector<int>> mass(SIZE1, std::vector<int>(SIZE2, 0));
  int iter = 0;
  while (true) {
    iter++;
    for (int i = 0; i < 20; i++) {
      int r = rnd.next(SIZE1);
      for (int i = 0; i < SIZE2; i++) {
        mass[r][i] = rnd.next(0, 100000);
      }
    }
    printf("iter:%d\n", iter);
    sleep(1);
  }
}
