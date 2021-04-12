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

#ifndef NOGDI
#	undef ERROR
#endif

#include <cstdio>
#include <cstdarg>
#include <string>
#include <vector>

namespace win32 {

Exception::Exception(const std::string &function_name, DWORD error) noexcept
		: function_name_(function_name), error_(error) {
}

const std::string& Exception::function_name() const noexcept {
	return function_name_;
}

DWORD Exception::error() const noexcept {
	return error_;
}

Exception1::Exception1(const std::string &function_name, DWORD error) noexcept
		: Exception(function_name, error),
		what_(function_name + ": " + hex_error(error)) {
}

const char *Exception1::what() const noexcept {
	return what_.c_str();
}

Exception2::Exception2(const std::string &function_name, DWORD return_code,
			DWORD error) noexcept
		: Exception(function_name, error), what_(function_name + ": "
			+ std::to_string(return_code_) + ", " + hex_error(error)),
		return_code_(return_code) {
}

const char *Exception2::what() const noexcept {
	return what_.c_str();
}

DWORD Exception2::return_code() const noexcept {
	return return_code_;
}

wrapped_ptr<HANDLE, ::CloseHandle> wrap_generic_handle(HANDLE handle) {
	return wrap_generic<HANDLE, ::CloseHandle>(handle);
}

wrapped_ptr<HANDLE, ::CloseHandle> wrap_file_handle(HANDLE handle) {
	if (handle == INVALID_HANDLE_VALUE) {
		handle = nullptr;
	}

	return wrap_generic_handle(handle);
}

wrapped_ptr<HANDLE, ::ReleaseMutex> acquire_mutex(HANDLE mutex, DWORD timeout_ms) {
	::SetLastError(0);
	DWORD ret = ::WaitForSingleObject(mutex, timeout_ms);
	if (ret == WAIT_OBJECT_0) {
		return wrap_generic<HANDLE, ::ReleaseMutex>(mutex);
	} else if (ret != WAIT_TIMEOUT) {
		throw win32::Exception2{"WaitForSingleObject(acquire_mutex)", ret};
	} else {
		return {};
	}
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
#else
std::string to_string(const native_char *text, ssize_t len) {
	if (len < 0) {
		return std::string{text};
	} else {
		return std::string{text, (size_t)len};
	}
}

std::string unicode_to_ansi_string(const wchar_t *text, ssize_t wlen) {
	int len = ::WideCharToMultiByte(CP_ACP, 0, text, wlen, nullptr, 0, nullptr, nullptr);
	if (len > 0) {
		auto buf = std::vector<char>(len);

		len = ::WideCharToMultiByte(CP_ACP, 0, text, wlen, buf.data(), buf.size(), nullptr, nullptr);

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
	return text;
}
#endif

std::string hex_error(DWORD error) noexcept {
	std::vector<char> text(2 + (sizeof(DWORD) * 2) + 1);

	std::snprintf(text.data(), text.size(), "0x%08lX", error);

	return {text.data()};
}

void log(HANDLE event_source, WORD type, WORD category, DWORD id,
		int argc, const char *format...) noexcept {
	std::va_list argv;
	va_start(argv, format);
	vlog(event_source, type, category, id, nullptr, argc, format, argv);
	va_end(argv);
}

void vlog(HANDLE event_source, WORD type, WORD category, DWORD id,
		const native_string *prefix, int argc, const char *format,
		std::va_list argv, bool console) noexcept {
	std::va_list args_fallback;
	va_copy(args_fallback, argv);

	int prefix_count = (prefix != nullptr) ? 1 : 0;
	std::vector<std::vector<win32::native_char>> ev_strings;
	std::vector<const win32::native_char*> ev_string_ptrs(prefix_count + argc);

	ev_strings.reserve(prefix_count + argc);
	if (prefix != nullptr) {
		ev_strings.emplace_back(prefix->c_str(), prefix->c_str() + prefix->length() + 1);
		ev_string_ptrs[0] = ev_strings.back().data();
	}

	for (int i = 0; i < argc; i++) {
		auto value = win32::ascii_to_native_string(va_arg(argv, char*));

		ev_strings.emplace_back(value.c_str(), value.c_str() + value.length() + 1);
		ev_string_ptrs[prefix_count + i] = ev_strings.back().data();
	}

	if (event_source != nullptr) {
		::ReportEvent(event_source, type, category, id, nullptr,
			ev_string_ptrs.size(), 0, ev_string_ptrs.data(), nullptr);
	}

	if (console) {
		auto &out = (type == EVENTLOG_INFORMATION_TYPE) ? cout : cerr;

		::SetLastError(0);
		auto formatted_text = win32::wrap_output<LPTSTR, ::LocalFree>(
			[&] (LPTSTR &data) {
				return FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
						| FORMAT_MESSAGE_FROM_HMODULE
						| FORMAT_MESSAGE_ARGUMENT_ARRAY,
					nullptr, static_cast<DWORD>(id), 0,
					reinterpret_cast<LPTSTR>(&data), 0,
					reinterpret_cast<va_list*>(const_cast<DWORD_PTR*>(reinterpret_cast<const DWORD_PTR*>(ev_string_ptrs.data()))));
			});
		if (formatted_text) {
			out << formatted_text.get() << std::flush;
		} else {
			std::vector<char> text(win32::max_path + 1024);

			if (std::vsnprintf(text.data(), text.size(), format, args_fallback) < 0) {
				text[0] = '?';
				text[1] = '\0';
			}

			if (prefix != nullptr) {
				out << *prefix << ": ";
			}
			out << text.data() << std::endl;
		}
	}
	::va_end(args_fallback);
}

native_string current_process_filename() {
	std::vector<native_char> filename(max_path + 1);

	::SetLastError(0);
	DWORD ret = ::GetModuleFileName(nullptr, filename.data(), filename.size());
	if (ret == 0 || ret == ERROR_INSUFFICIENT_BUFFER) {
		throw Exception1{"GetModuleFileName"};
	}

	return {filename.data()};
}

bool is_elevated() {
	::SetLastError(0);
	auto admins = wrap_output<PSID, ::FreeSid>(
		[&] (PSID &data) {
			SID_IDENTIFIER_AUTHORITY authority = SECURITY_NT_AUTHORITY;
			return ::AllocateAndInitializeSid(&authority, 2,
				SECURITY_BUILTIN_DOMAIN_RID,
				DOMAIN_ALIAS_RID_ADMINS,
				0, 0, 0, 0, 0, 0, &data);
		});
	if (!admins) {
		throw Exception1{"AllocateAndInitializeSid"};
	}

	// Determine whether the SID of administrators group is enabled in
	// the primary access token of the process.
	BOOL admin = false;
	::SetLastError(0);
	if (!::CheckTokenMembership(nullptr, admins.get(), &admin)) {
		throw Exception1{"CheckTokenMembership"};
	}

	return admin;
}

int run_elevated(const std::vector<native_string> &parameters) {
	auto filename = current_process_filename();
	native_string cmdline;
	bool first = true;

	cmdline.reserve(max_path);
	for (auto& parameter : parameters) {
		if (!first) {
			cmdline += ' ';
		}
		first = false;

		cmdline += '\"';
		for (auto& c : parameter) {
			if (c == '\"') {
				cmdline += '\"';
			}
			cmdline += c;
		}
		cmdline += '\"';
	}

	SHELLEXECUTEINFO exec_info{};
	exec_info.cbSize = sizeof(exec_info);
	exec_info.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
	exec_info.hwnd = nullptr;
	exec_info.lpVerb = TEXT("runas");
	exec_info.lpFile = filename.c_str();
	exec_info.lpParameters = cmdline.c_str();
	exec_info.lpDirectory = nullptr;
	exec_info.nShow = SW_SHOWDEFAULT;
	exec_info.hInstApp = 0;
	exec_info.hProcess = nullptr;

	::SetLastError(0);
	if (!::ShellExecuteEx(&exec_info)) {
		throw Exception1{"ShellExecuteEx"};
	}

	auto process = wrap_generic_handle(exec_info.hProcess);
	if (process) {
		::SetLastError(0);
		DWORD ret = ::WaitForSingleObject(process.get(), INFINITE);
		if (ret != WAIT_OBJECT_0) {
			throw Exception2{"WaitForSingleObject(run_elevated)", ret};
		}

		DWORD exit_code = 0;
		::SetLastError(0);
		if (!::GetExitCodeProcess(process.get(), &exit_code)) {
			throw Exception1{"GetExitCodeProcess"};
		}

		return exit_code;
	}

	return 0;
}


} /* namespace win32 */
