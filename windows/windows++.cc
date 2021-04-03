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
#include <stdexcept>
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

wrapped_ptr<HKEY, ::RegCloseKey> wrap_reg_key(HKEY key) {
	return wrap_generic<HKEY, ::RegCloseKey>(key);
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

native_string current_process_filename() {
	std::vector<native_char> filename(max_path + 1);

	::SetLastError(0);
	DWORD ret = ::GetModuleFileName(nullptr, filename.data(), filename.size());
	if (ret == 0 || ret == ERROR_INSUFFICIENT_BUFFER) {
		throw std::runtime_error{"GetModuleFileName returned " + win32::last_error()};
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
		throw std::runtime_error{"AllocateAndInitializeSid returned " + win32::last_error()};
	}

	// Determine whether the SID of administrators group is enabled in
	// the primary access token of the process.
	BOOL admin = false;
	::SetLastError(0);
	if (!::CheckTokenMembership(nullptr, admins.get(), &admin)) {
		throw std::runtime_error{"CheckTokenMembership returned " + win32::last_error()};
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
		throw std::runtime_error{"ShellExecuteEx returned " + win32::last_error()};
	}

	auto process = wrap_generic_handle(exec_info.hProcess);
	if (process) {
		::SetLastError(0);
		DWORD ret = ::WaitForSingleObject(process.get(), INFINITE);
		if (ret != WAIT_OBJECT_0) {
			throw std::runtime_error{"WaitForSingleObject returned "
				+ std::to_string(ret) + ", " + win32::last_error()};
		}

		DWORD exit_code = 0;
		::SetLastError(0);
		if (!::GetExitCodeProcess(process.get(), &exit_code)) {
			throw std::runtime_error{"GetExitCodeProcess returned " + win32::last_error()};
		}

		return exit_code;
	}

	return 0;
}


} /* namespace win32 */
