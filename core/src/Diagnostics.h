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

const std::list<std::string>& get();

#ifdef ZX_DIAGNOSTICS

bool enabled();
void setEnabled(const bool enabled);

void begin();
void moveTo(std::list<std::string>& diagnostics);
void clear();

void put(const std::string& value);
void put(const int value);
void put(const ByteArray& value, int begin = -1, int end = -1);
void fmt(const char* const format, ...);
void chr(const unsigned char value, const char* const prefixIfNonASCII = "", const bool appendHex = true);
void dump(const std::vector<int> value, const char* const postfix = "", int begin = -1, int end = -1, bool hex = false);
void dump(const ByteArray& value, const char* const postfix = "", int begin = -1, int end = -1, bool hex = false);

std::string print(const std::list<std::string>* p_diagnostics, bool skipToDecode = false);

void put(std::list<std::string>* p_diagnostics, const std::string& value);
void put(std::list<std::string>* p_diagnostics, const int value);
void put(std::list<std::string>* p_diagnostics, const ByteArray& value, int begin = -1, int end = -1);
void fmt(std::list<std::string>* p_diagnostics, const char* const format, ...);
void chr(std::list<std::string>* p_diagnostics, const unsigned char value, const char* const prefixIfNonASCII = "",
		 const bool appendHex = false);

#else /* ZX_DIAGNOSTICS */

static inline bool enabled() { return false; }
static inline void setEnabled(const bool /*enabled*/) {}

static inline void begin() {}
static inline void moveTo(std::list<std::string>& /*diagnostics*/) {}
static inline void clear() {}

static inline void put(const std::string& /*value*/) {}
static inline void put(const int /*value*/) {}
static inline void put(const ByteArray& /*value*/, int /*begin*/ = -1, int /*end*/ = -1) {}
static inline void fmt(const char* const /*format*/, ...) {}
static inline void chr(const unsigned char /*value*/, const char* const /*prefixIfNonASCII*/ = "", const bool /*appendHex*/ = true) {}
static inline void dump(const std::vector<int> /*value*/, const char* const /*postfix*/ = "", int /*begin*/ = -1, int /*end*/ = -1, bool /*hex*/ = false) {}
static inline void dump(const ByteArray& /*value*/, const char* const /*postfix*/ = "", int /*begin*/ = -1, int /*end*/ = -1, bool /*hex*/ = false) {}

static inline std::string print(const std::list<std::string>* /*p_diagnostics*/, bool /*skipToDecode*/ = false) { return std::string(); }

static inline void put(std::list<std::string>* /*p_diagnostics*/, const std::string& /*value*/) {}
static inline void put(std::list<std::string>* /*p_diagnostics*/, const int /*value*/) {}
static inline void put(std::list<std::string>* /*p_diagnostics*/, const ByteArray& /*value*/, int /*begin*/ = -1, int /*end*/ = -1) {}
static inline void fmt(std::list<std::string>* /*p_diagnostics*/, const char* const /*format*/, ...) {}
static inline void chr(std::list<std::string>* /*p_diagnostics*/, const unsigned char /*value*/, const char* const /*prefixIfNonASCII*/ = "",
		 const bool /*appendHex*/ = false) {}

#endif /* ZX_DIAGNOSTICS */

} // namespace Diagnostics
} // namespace ZXing
