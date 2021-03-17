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
#include <sstream>

namespace ZXing {

void Diagnostics::put(const std::vector<int> value, const char* const postfix, int begin, int end)
{
	if (_enabled) {
		std::ostringstream s;
		if (begin == -1) {
			begin = 0;
		}
		if (end == -1) {
			end = value.size();
		}
		for (int i = begin; i < end; i++) {
			s << value[i];
			if (i != end - 1) {
				s << " ";
			}
		}
		s << postfix;
		put(s.str());
	}
}

// https://stackoverflow.com/a/49812018/664741
void Diagnostics::fmt(const char* const format, ...)
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
		const int iLen = std::vsnprintf(NULL, 0, format, vaArgsCopy);
		va_end(vaArgsCopy);

		// create a formatted string without risking memory mismanagement
		// and without assuming any compiler or platform specific behavior
		std::vector<char> zc(iLen + 1);
		std::vsnprintf(zc.data(), zc.size(), format, vaArgs);
		va_end(vaArgs);
		put(std::string(zc.data(), iLen));
	}
}

void Diagnostics::chr(const unsigned char value, const char* const prefixIfNonASCII, const bool appendHex)
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

} // ZXing
