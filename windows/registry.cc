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
#include <windows.h>
#include <ktmw32.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include "events.h"
#include "hid-identify.h"
#include "../common/types.h"
#include "windows++.h"

namespace hid_identify {

static const win32::native_string LOG_REG_HLKM_KEY = TEXT("SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\") + LOG_PROVIDER;
static const win32::native_string LOG_REG_VALUE_CATFILE_NAME = TEXT("CategoryMessageFile");
static const win32::native_string LOG_REG_VALUE_CATCOUNT_NAME = TEXT("CategoryCount");
static const DWORD LOG_REG_VALUE_CATCOUNT_DATA = LOGGING_CATEGORY_MAX;
static const win32::native_string LOG_REG_VALUE_MSGFILE_NAME = TEXT("EventMessageFile");
static const win32::native_string LOG_REG_VALUE_TYPES_NAME = TEXT("TypesSupported");
static const DWORD LOG_REG_VALUE_TYPES_DATA = EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_ERROR_TYPE;

static void registry_set_value(HKEY key, const win32::native_string &name,
		DWORD type, const BYTE *data, DWORD len, bool verbose) {
	DWORD ret = ::RegSetValueEx(key, name.c_str(), 0, type, data, len);

	if (ret != ERROR_SUCCESS) {
		win32::cerr << "RegSetValueEx for " << name << " returned "
			<< win32::ascii_to_native_string(win32::hex_error(ret)) << std::endl;
		throw OSError{};
	}

	if (verbose) {
		win32::cout << "Set value of " << LOG_REG_VALUE_MSGFILE_NAME << std::endl;
	}
}

void registry_add_event_log(bool verbose) {
	auto txn = win32::wrap_file_handle(::CreateTransaction(
		nullptr, nullptr, 0, 0, 0, 60000, nullptr));

	auto log_key = win32::wrap_output<HKEY, ::RegCloseKey>(
		[&] (HKEY &data) {
			DWORD disposition = 0;
			LSTATUS ret = ::RegCreateKeyTransacted(HKEY_LOCAL_MACHINE,
				LOG_REG_HLKM_KEY.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE,
				KEY_ALL_ACCESS, nullptr, &data, &disposition, txn.get(), nullptr);

			if (ret != ERROR_SUCCESS) {
				win32::cerr << "RegCreateKeyTransacted returned "
					<< win32::ascii_to_native_string(win32::hex_error(ret)) << std::endl;
				throw OSError{};
			}

			switch (disposition) {
			case REG_CREATED_NEW_KEY:
				if (verbose) {
					win32::cout << "Created new key: " << LOG_REG_HLKM_KEY << std::endl;
				}
				break;

			case REG_OPENED_EXISTING_KEY:
				if (verbose) {
					win32::cout << "Key already exists: " << LOG_REG_HLKM_KEY << std::endl;
				}
				break;
			}

			return true;
		});

	auto filename = win32::current_process_filename();

	registry_set_value(log_key.get(), LOG_REG_VALUE_CATFILE_NAME,
		REG_SZ, reinterpret_cast<const BYTE*>(filename.c_str()),
		(filename.length() + 1) * sizeof(win32::native_char), verbose);

	registry_set_value(log_key.get(), LOG_REG_VALUE_CATCOUNT_NAME,
		REG_DWORD, reinterpret_cast<const BYTE*>(&LOG_REG_VALUE_CATCOUNT_DATA),
		sizeof(LOG_REG_VALUE_CATCOUNT_DATA), verbose);

	registry_set_value(log_key.get(), LOG_REG_VALUE_MSGFILE_NAME,
		REG_SZ, reinterpret_cast<const BYTE*>(filename.c_str()),
		(filename.length() + 1) * sizeof(win32::native_char), verbose);

	registry_set_value(log_key.get(), LOG_REG_VALUE_TYPES_NAME,
		REG_DWORD, reinterpret_cast<const BYTE*>(&LOG_REG_VALUE_TYPES_DATA),
		sizeof(LOG_REG_VALUE_TYPES_DATA), verbose);

	SetLastError(0);
	if (!::CommitTransaction(txn.get())) {
		win32::cerr << "CommitTransaction returned "
			<< win32::ascii_to_native_string(win32::last_error()) << std::endl;
		throw OSError{};
	}
	if (verbose) {
		win32::cout << "Committed changes" << std::endl;
	}
}

void registry_remove_event_log(bool verbose) {
	auto txn = win32::wrap_file_handle(::CreateTransaction(
		nullptr, nullptr, 0, 0, 0, 60000, nullptr));

	DWORD ret = ::RegDeleteKeyTransacted(HKEY_LOCAL_MACHINE, LOG_REG_HLKM_KEY.c_str(),
		0, 0, txn.get(), nullptr);
	if (ret == ERROR_SUCCESS) {
		if (verbose) {
			win32::cout << "Deleted key: " << LOG_REG_HLKM_KEY << std::endl;
		}
	} else if (ret == ERROR_FILE_NOT_FOUND) {
		if (verbose) {
			win32::cout << "Key not found: " << LOG_REG_HLKM_KEY << std::endl;
		}
	} else {
		win32::cerr << "RegDeleteKeyTransacted returned "
			<< win32::ascii_to_native_string(win32::hex_error(ret)) << std::endl;
		throw OSError{};
	}

	SetLastError(0);
	if (!::CommitTransaction(txn.get())) {
		win32::cerr << "CommitTransaction returned "
			<< win32::ascii_to_native_string(win32::last_error()) << std::endl;
		throw OSError{};
	}

	if (verbose) {
		win32::cout << "Committed changes" << std::endl;
	}
}

} // namespace hid_identify
