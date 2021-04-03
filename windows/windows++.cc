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
#include "windows++.h"

#include <windows.h>

#include <cstdio>
#include <vector>

namespace win32 {

wrapped_ptr<HANDLE, ::CloseHandle> wrap_generic_handle(HANDLE handle) {
	return wrap_generic<HANDLE, ::CloseHandle>(handle);
}

wrapped_ptr<HANDLE, ::CloseHandle> wrap_file_handle(HANDLE handle) {
	if (handle == INVALID_HANDLE_VALUE) {
		handle = nullptr;
	}

	return wrap_generic_handle(handle);
}

#ifdef UNICODE
std::string to_string(const native_char *text, ssize_t wlen) {
	int len = ::WideCharToMultiByte(CP_UTF8, 0, text, wlen, nullptr, 0, nullptr, nullptr);
	if (len > 0) {
		auto buf = std::vector<char>(len);

		len = ::WideCharToMultiByte(CP_UTF8, 0, text, wlen, buf.data(), buf.size(), nullptr, nullptr);

		if (wlen != -1) {
			return std::string{buf.data(), (size_t)len};
		} else {
			return std::string{buf.data()};
		}
	} else {
		return "";
	}
}

native_string ascii_to_native_string(const std::string &text) {
	return native_string{text.begin(), text.end()};
}

native_string to_native_string(const sized_data<BYTE, DWORD> &data) {
	return native_string{reinterpret_cast<native_char*>(data.get()), data.size()/2};
}
#else
std::string to_string(const native_char *text, ssize_t len) {
	if (len < 0) {
		return std::string{text};
	} else {
		return std::string{text, (size_t)len};
	}
}

native_string ascii_to_native_string(const std::string &text) {
	return text;
}

native_string to_native_string(const sized_data<BYTE, DWORD> &data) {
	return native_string{reinterpret_cast<native_char*>(data.get()), data.size()};
}
#endif

std::vector<native_string> reg_multi_sz(const native_string &text) {
	std::vector<native_string> values;
	native_string null_value{TEXT(""), 1};
	size_t pos = 0;
	size_t match = text.find(null_value);

	while (match != native_string::npos && match != text.length() - 1) {
		values.emplace_back(text.substr(pos, match - pos));
		pos = match + 1;
		match = text.find(null_value, pos);
	}

	return values;
}

std::string hex_error(DWORD error) {
	std::vector<char> text(2 + (sizeof(DWORD) * 2) + 1);

	std::snprintf(text.data(), text.size(), "0x%08lX", error);

	return {text.data()};
}

std::string last_error() {
	return hex_error(GetLastError());
}

} /* namespace win32 */
