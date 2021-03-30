/*
  qmk-hid-identify - Identify the current OS to QMK device
  Copyright 2021  Simon Arlott

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
// Modified from https://android.googlesource.com/platform/system/core/+/c0e6e40/base/include/android-base/unique_fd.h
// blob 97406be84f1d5b93d664a25399d31c45e5502410
// commit c0e6e40cc916747a0a22c2538874348cda0ef607
/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <unistd.h>

namespace hid_identify {

// Container for a file descriptor that automatically closes the descriptor as
// it goes out of scope.
//
//      unique_fd ufd(open("/some/path", "r"));
//      if (ufd.get() == -1) return error;
//
//      // Do something useful, possibly including 'return'.
//
//      return 0; // Descriptor is closed for you.
class unique_fd final {
 public:
  unique_fd() : value_(-1) {}
  explicit unique_fd(int value) : value_(value) {}
  ~unique_fd() { clear(); }
  unique_fd(unique_fd&& other) : value_(other.release()) {}
  unique_fd& operator=(unique_fd&& s) {
    reset(s.release());
    return *this;
  }
  void reset(int new_value) {
    if (value_ != -1) {
      // Even if close(2) fails with EINTR, the fd will have been closed.
      // Using TEMP_FAILURE_RETRY will either lead to EBADF or closing someone else's fd.
      // http://lkml.indiana.edu/hypermail/linux/kernel/0509.1/0877.html
      close(value_);
    }
    value_ = new_value;
  }
  void clear() {
    reset(-1);
  }
  int get() const { return value_; }
  int release() __attribute__((warn_unused_result)) {
    int ret = value_;
    value_ = -1;
    return ret;
  }
  explicit operator bool() const { return value_ != -1; }
 private:
  int value_;
  unique_fd(const unique_fd&) = delete;
  void operator=(const unique_fd&) = delete;
};

} // namespace hid_identify
