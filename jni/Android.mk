# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_CFLAGS    := -std=c++14 -Wall -g -D_FILE_OFFSET_BITS=64 -D__IS_NDK_BUILD__=1 -O2 -fvisibility=hidden
LOCAL_MODULE    := mempatch
LOCAL_SRC_FILES := main.cpp Patcher.cpp ChangeString.cpp Memory_Linux.cpp Utility.cpp Converter.cpp Address.cpp LineReader.cpp linenoise/linenoise.cpp FreezeThread.cpp
LOCAL_SRC_FILES += SnappedRange.cpp
LOCAL_LDLIBS    := -llog -latomic
LOCAL_CFLAGS    += -fPIE
LOCAL_LDFLAGS   += -fPIE -pie -pthread
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS    := -std=c++14 -Wall -g -D__IS_NDK_BUILD__=1 -O0 -fvisibility=hidden
LOCAL_MODULE    := victim_int
LOCAL_SRC_FILES := test/victim_int.cpp
LOCAL_LDLIBS    := -llog
LOCAL_CFLAGS    += -fPIE
LOCAL_LDFLAGS   += -fPIE -pie
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_CFLAGS    := -std=c++14 -Wall -g -D__IS_NDK_BUILD__=1 -O0 -fvisibility=hidden
LOCAL_MODULE    := victim_ascii
LOCAL_SRC_FILES := test/victim_ascii.cpp
LOCAL_LDLIBS    := -llog
LOCAL_CFLAGS    += -fPIE
LOCAL_LDFLAGS   += -fPIE -pie
include $(BUILD_EXECUTABLE)

