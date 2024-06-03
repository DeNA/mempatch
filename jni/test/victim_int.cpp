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
#include <stdint.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    int32_t sum = 0;
    int32_t step = 123456789;
    int32_t sleep_time = 5 * 1000 * 1000;
    if (argc > 1) {
        step = atoi(argv[1]);
    }
    if (argc > 2) {
        sleep_time = atoi(argv[2]);
    }
    while (true) {
        sum += step;
        fprintf(stdout, "step=%d, sleep=%d(micro sec), sum=%d\n", step, sleep_time, sum);
        usleep(sleep_time);
    }
}
