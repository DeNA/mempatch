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
#include "getopt.h"
#include <cstring>
#include <cstdio>

char *optarg = nullptr;
int optind = 1, opterr = 1, optopt = '?';

int getopt(int argc, char *const argv[], const char *optstring) {
    static char *next = nullptr;
    if (optind == 1) {
        next = nullptr;
    }

    optarg = nullptr;

    if (next == nullptr || *next == '\0') {
        if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0') {
            return -1;
        }

        if (strcmp(argv[optind], "--") == 0) {
            optind++;
            return -1;
        }

        next = argv[optind] + 1;
        optind++;
    }

    char c = *next++;
    const char *opt = strchr(optstring, c);

    if (opt == nullptr || c == ':') {
        optopt = c;
        if (opterr) {
            fprintf(stderr, "Unknown option `-%c'.\n", c);
        }
        return '?';
    }

    if (opt[1] == ':') {
        if (*next != '\0') {
            optarg = next;
            next = nullptr;
        } else if (optind < argc) {
            optarg = argv[optind];
            optind++;
        } else {
            optopt = c;
            if (opterr) {
                fprintf(stderr, "Option `-%c' requires an argument.\n", c);
            }
            return '?';
        }
    }

    return c;
}
