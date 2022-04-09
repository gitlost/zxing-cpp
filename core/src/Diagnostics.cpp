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

#include "Diagnostics.h"

#include <cstdarg>
#include <iomanip>
#include <sstream>

namespace ZXing::Diagnostics {

static thread_local bool _enabled = false;
static thread_local std::list<std::string> _contents;

bool enabled()
{
	return _enabled;
}

void setEnabled(const bool enabled)
{
	_enabled = enabled;
	_contents.clear();
}

const std::list<std::string>& get()
{
	return _contents;
}

void moveTo(std::list<std::string>& diagnostics)
{
	diagnostics = std::move(_contents);
	_contents.clear();
}

void clear()
{
	_contents.clear();
}

void put(const std::string& value)
{
	if (_enabled && !value.empty()) {
		_contents.push_back(value);
	}
}

void put(const int value)
{
	if (_enabled) {
		_contents.push_back(std::to_string(value));
	}
}

void put(const ByteArray& value, int begin, int end)
{
	if (_enabled) {
		if (begin == -1) {
			begin = 0;
		}
		if (end == -1) {
			end = static_cast<int>(value.size());
		}
		for (int i = begin; i < end; i++) {
			chr(value[i], "", true/*appendHex*/);
		}
	}
}

// https://stackoverflow.com/a/49812018/664741
void fmt(const char* const format, ...)
{
	if (_enabled) {
		// initialize use of the variable argument array
		va_list vaArgs;
		va_start(vaArgs, format);

		// reliably acquire the size
		// from a copy of the variable argument array
		// and a functionally reliable call to mock the formatting
		va_list vaArgsCopy;
		va_copy(vaArgsCopy, vaArgs);
		const int iLen = std::vsnprintf(nullptr, 0, format, vaArgsCopy);
		va_end(vaArgsCopy);

		// create a formatted string without risking memory mismanagement
		// and without assuming any compiler or platform specific behavior
		std::vector<char> zc(iLen + 1);
		std::vsnprintf(zc.data(), zc.size(), format, vaArgs);
		va_end(vaArgs);
		put(std::string(zc.data(), iLen));
	}
}

void chr(const unsigned char value, const char* const prefixIfNonASCII, const bool appendHex)
{
	if (_enabled) {
		static const char* const ascii_nongraphs[34] = {
			"NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
			 "BS",  "HT",  "LF",  "VT",  "FF",  "CR",  "SO",  "SI",
			"DLE", "DC1", "DC2", "DC3", "DC4", "NAK", "SYN", "ETB",
			"CAN",  "EM", "SUB", "ESC",  "FS",  "GS",  "RS",  "US",
			 "SP", "DEL",
		};
		std::ostringstream s;

		if (value > 32 && value < 127) { // Graphical ASCII
			s << value;
		}
		else if (value < 128) { // Non-graphical ASCII
			s << "<" << ascii_nongraphs[value == 127 ? 33 : value] << ">";
		}
		else { // Non-ASCII
			s << prefixIfNonASCII << static_cast<unsigned int>(value);
			if (appendHex) {
				s << "(0x" << std::uppercase << std::hex << static_cast<unsigned int>(value) << ")";
			}
		}

		put(s.str());
	}
}

void dump(const std::vector<int> value, const char* const postfix, int begin, int end, bool hex)
{
	if (_enabled) {
		std::ostringstream s;
		s.fill('0');

		if (begin == -1) {
			begin = 0;
		}
		if (end == -1) {
			end = static_cast<int>(value.size());
		}
		for (int i = begin; i < end; i++) {
			if (hex) {
				s << std::setw(2) << std::hex << value[i];
			} else {
				s << value[i];
			}
			if (i != end - 1) {
				s << " ";
			}
		}
		s << postfix;
		put(s.str());
	}
}

void dump(const ByteArray& value, const char* const postfix, int begin, int end, bool hex)
{
	if (_enabled) {
		std::vector<int> v;
		v.insert(v.end(), value.begin(), value.end());
		dump(v, postfix, begin, end, hex);
	}
}

} // ZXing
