#pragma once
/*
* Copyright 2021 gitlost
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

#include "ByteArray.h"

#include <list>
#include <string>
#include <vector>

namespace ZXing {

namespace Diagnostics {

bool enabled();
void setEnabled(const bool enabled);

const std::list<std::string>& get();
void moveTo(std::list<std::string>& diagnostics);
void clear();

void put(const std::string& value);
void put(const int value);
void put(const ByteArray& value, int begin = -1, int end = -1);
void fmt(const char* const format, ...);
void chr(const unsigned char value, const char* const prefixIfNonASCII = "", const bool appendHex = false);
void dump(const std::vector<int> value, const char* const postfix = "", int begin = -1, int end = -1, bool hex = false);
void dump(const ByteArray& value, const char* const postfix = "", int begin = -1, int end = -1, bool hex = false);

} // namespace Diagnostics
} // namespace ZXing
