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
#include <exception>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace win32 {

constexpr DWORD max_path = 32767;

class Exception: public std::exception {
public:
	const std::string& function_name() const noexcept;
	DWORD error() const noexcept;

protected:
	Exception(const std::string &function_name, DWORD error) noexcept;

private:
	std::string function_name_;
	DWORD error_;
};

class Exception1: public Exception {
public:
	Exception1(const std::string &function_name,
		DWORD error = ::GetLastError()) noexcept;

	virtual const char *what() const noexcept;

private:
	std::string what_;
};

class Exception2: public Exception {
public:
	Exception2(const std::string &function_name, DWORD return_code,
		DWORD error = ::GetLastError()) noexcept;

	virtual const char *what() const noexcept;
	DWORD return_code() const noexcept;

private:
	std::string what_;
	DWORD return_code_;
};

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

wrapped_ptr<HANDLE, ::ReleaseMutex> acquire_mutex(HANDLE mutex, DWORD timeout_ms = INFINITE);

/* https://stackoverflow.com/q/2898228/388191 */
inline bool isdigit(wchar_t ch) {
	return (ch >= '0' && ch <= '9');
}

/* https://stackoverflow.com/q/2898228/388191 */
inline bool isxdigit(wchar_t ch) {
	return isdigit(ch) || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

std::string hex_error(DWORD error) noexcept;

void log(HANDLE event_source, WORD type, WORD category, DWORD id,
	int argc, const char *format...) noexcept;

void vlog(HANDLE event_source, WORD type, WORD category, DWORD id,
		const std::wstring *prefix, int argc, const char *format,
		std::va_list argv, bool console = true) noexcept;

std::wstring current_process_filename();
bool is_elevated();
int run_elevated(const std::vector<std::wstring> &parameters);

} /* namespace win32 */
