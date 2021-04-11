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
#pragma once

#include <windows.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include <cstdarg>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace win32 {

constexpr DWORD max_path = 32767;

template <class T>
using remove_pointer_t = typename std::remove_pointer<T>::type;

/* https://stackoverflow.com/a/51274008 */
template <auto fn>
using deleter_from_fn = std::integral_constant<decltype(fn), fn>;

template <typename T, auto fn>
using wrapped_data = std::unique_ptr<T, deleter_from_fn<fn>>;

template <typename T, auto fn>
using wrapped_ptr = std::unique_ptr<remove_pointer_t<T>, deleter_from_fn<fn>>;

template <typename T>
wrapped_data<T, std::free> make_generic(size_t size) {
	/* Can't be converted to use array allocation because at least one
	 * usage allocates a single struct with a size specified in bytes.
	 */
	T* data = static_cast<T*>(std::calloc(1, size));

	if (data == nullptr && size > 0) {
		throw std::bad_alloc{};
	}

	/* Not using wrap_generic() because size may be 0 causing data to be nullptr */
	return wrapped_data<T, std::free>{data};
}

template <typename T, auto Deleter>
wrapped_ptr<T, Deleter> wrap_generic(T data) {
	if (data != nullptr) {
		return wrapped_ptr<T, Deleter>{data};
	} else {
		return {};
	}
}

wrapped_ptr<HANDLE, ::CloseHandle> wrap_generic_handle(HANDLE handle);
wrapped_ptr<HANDLE, ::CloseHandle> wrap_file_handle(HANDLE handle);
wrapped_ptr<HKEY, ::RegCloseKey> wrap_reg_key(HKEY key);

template <typename T>
using output_func_t = std::function<BOOLEAN(T &data)>;

template <typename T, auto Deleter>
wrapped_ptr<T, Deleter> wrap_output(
		output_func_t<T> output_func) {
	T data = nullptr;

	try {
		if (!output_func(data)) {
			data = nullptr;
		}
	} catch (...) {
		if (data != nullptr) {
			Deleter(data);
		}
		throw;
	}

	return wrap_generic<T, Deleter>(data);
}

template <class T, class Size>
class sized_data: public wrapped_data<T, std::free> {
public:
	sized_data() : wrapped_data<T, std::free>(), size_(0) {}

	explicit sized_data(wrapped_data<T, std::free> &ptr, Size size)
			: wrapped_data<T, std::free>(std::move(ptr)), size_(size) {}

	Size size() const {
		return size_;
	}

private:
	Size size_;
};

template <class T, class Size>
using sized_func_t = std::function<BOOLEAN(T *data, Size size, Size *required_size)>;

#ifdef UNICODE
using native_string = std::wstring;
static constexpr std::wistream &cin = std::wcin;
static constexpr std::wostream &cout = std::wcout;
static constexpr std::wostream &cerr = std::wcerr;
static constexpr std::wostream &clog = std::wclog;

std::string to_string(const wchar_t *text, ssize_t wlen = -1);
#else
using native_string = std::string;
static constexpr std::istream &cin = std::cin;
static constexpr std::ostream &cout = std::cout;
static constexpr std::ostream &cerr = std::cerr;
static constexpr std::ostream &clog = std::clog;

std::string to_string(const char *text, ssize_t len = -1);
#endif

template <class T, class Size>
sized_data<T, Size> make_sized(sized_func_t<T, Size> sized_func) {
	Size required_size = 0;

	::SetLastError(0);
	auto ret = sized_func(nullptr, 0, &required_size);
	if (!ret && ::GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		return {};
	}

	Size size = required_size;
	auto data = make_generic<T>(size);

	::SetLastError(0);
	ret = sized_func(data.get(), size, &required_size);
	if (!ret) {
		return {};
	}

	return sized_data<T, Size>{data, size};
}

using native_char = typename native_string::value_type;

native_string ascii_to_native_string(const std::string &text);

native_string to_native_string(const sized_data<BYTE, DWORD> &data);

/* https://stackoverflow.com/q/2898228/388191 */
inline bool isdigit(native_char ch) {
	return (ch >= '0' && ch <= '9');
}

/* https://stackoverflow.com/q/2898228/388191 */
inline bool isxdigit(native_char ch) {
	return isdigit(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

std::vector<native_string> reg_multi_sz(const native_string &text);

std::string hex_error(DWORD error);

void log(HANDLE event_source, WORD type, WORD category, DWORD id,
	int argc, const char *format...);

void vlog(HANDLE event_source, WORD type, WORD category, DWORD id,
		const native_string *prefix, int argc, const char *format,
		std::va_list argv, bool console = true);

native_string current_process_filename();
bool is_elevated();
int run_elevated(const std::vector<native_string> &parameters);

} /* namespace win32 */
