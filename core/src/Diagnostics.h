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

#include <list>
#include <string>
#include <vector>

namespace ZXing {

class Diagnostics
{
public:
	Diagnostics(const bool enabled) : _enabled(enabled) {}

	bool enabled() const { return _enabled; }
	std::list<std::string>& get() { return _contents; }

	void put(const std::string& value) { if (_enabled && !value.empty()) _contents.push_back(value); }
	void put(const int value) { if (_enabled) _contents.push_back(std::to_string(value)); }
	void put(const std::vector<int> value, const char* const postfix = "", int begin = -1, int end = -1);
	void fmt(const char* const format, ...);
	void chr(const unsigned char value, const char* const prefixIfNonASCII = "", const bool appendHex = false);

private:
	bool _enabled = false;
	std::list<std::string> _contents;
};

} // ZXing
