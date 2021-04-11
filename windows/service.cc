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
#include "service.h"

#include <windows.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include "events.h"
#include "hid-identify.h"
#include "registry.h"
#include "../common/types.h"
#include "windows++.h"

#include <array>
#include <cstdarg>
#include <iostream>
#include <vector>

namespace hid_identify {

int command_service() {
	static WindowsHIDService service;
	std::vector<win32::native_char> name{SVC_KEY.c_str(), SVC_KEY.c_str() + SVC_KEY.length() + 1};
	std::array<SERVICE_TABLE_ENTRY, 2> dispatch_table{
		{
			{ name.data(), [] (DWORD, LPTSTR[]) {
					service.main();
				}
			},
			{ nullptr, nullptr }
		}
	};

	::SetLastError(0);
	if (!StartServiceCtrlDispatcher(dispatch_table.data())) {
		auto error = ::GetLastError();
		auto event_log = win32::wrap_generic<HANDLE, ::DeregisterEventSource>(
			RegisterEventSource(nullptr, LOG_PROVIDER.c_str()));
		win32::log(event_log.get(), EVENTLOG_ERROR_TYPE,
			LOGGING_CATEGORY_SERVICE_ID,
			LOGGING_MESSAGE_SVC_OS_FUNC_ERROR_CODE_1_ID,
			2, ::gettext("%s: %s"), "StartServiceCtrlDispatcher", win32::hex_error(error).c_str());
		throw OSError{};
	}

	return 0;
}

WindowsHIDService::WindowsHIDService() {
	::SetLastError(0);
	event_log_ = win32::wrap_generic<HANDLE, ::DeregisterEventSource>(
		RegisterEventSource(nullptr, LOG_PROVIDER.c_str()));
	if (!event_log_) {
		auto error = ::GetLastError();
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "RegisterEventSource", win32::hex_error(error).c_str());
		throw OSError{};
	}
}

void WindowsHIDService::main() {
	::SetLastError(0);
	status_ = ::RegisterServiceCtrlHandlerEx(SVC_KEY.c_str(),
		&WindowsHIDService::control_callback, this);
	if (status_ == 0) {
		auto error = ::GetLastError();
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "RegisterServiceCtrlHandlerEx", win32::hex_error(error).c_str());
		throw OSError{};
	}

	try {
		DWORD exit_code = run();
		if (exit_code == NO_ERROR) {
			status_ok(SERVICE_STOPPED);
		} else {
			status_error(exit_code);
		}
	} catch (...) {
		status_error(ERROR_SERVICE_SPECIFIC_ERROR, 1);
		throw;
	}
}

DWORD WindowsHIDService::run() {
	status_pending(SERVICE_START_PENDING, 5000, 1);

	::SetLastError(0);
	stop_event_ = win32::wrap_generic_handle(::CreateEvent(nullptr, true, false, nullptr));
	if (!stop_event_) {
		auto error = ::GetLastError();
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "CreateEvent", win32::hex_error(error).c_str());
		return error;
	}

	status_ok(SERVICE_RUNNING);

	while (1) {
		::SetLastError(0);
		DWORD ret = ::WaitForSingleObject(stop_event_.get(), INFINITE);
		if (ret != WAIT_OBJECT_0) {
			auto error = ::GetLastError();
			log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_OS_FUNC_ERROR_CODE_2,
				3, ::gettext("%s: %s, %s"), "WaitForSingleObject",
				std::to_string(ret), win32::hex_error(error).c_str());
			return error;
		} else {
			break;
		}
	}

	return NO_ERROR;
}

DWORD WindowsHIDService::control_callback(DWORD code, DWORD type,
		LPVOID event, LPVOID context) {
	WindowsHIDService *service = static_cast<WindowsHIDService*>(context);
	return service->control(code, type, event);
}

DWORD WindowsHIDService::control(DWORD code, DWORD type __attribute__((unused)),
		LPVOID event __attribute__((unused))) {
	switch (code) {
		case SERVICE_CONTROL_INTERROGATE:
			return NO_ERROR;

		case SERVICE_CONTROL_STOP:
			status_pending(SERVICE_STOP_PENDING, 5000, 1);
			::SetEvent(stop_event_.get());
			return NO_ERROR;

		default:
			return ERROR_CALL_NOT_IMPLEMENTED;
	}
}

void WindowsHIDService::report_status(DWORD state, DWORD exit_code,
		DWORD service_exit_code, DWORD wait_hint_ms, DWORD check_point) {
	SERVICE_STATUS status{};

	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = state;
	if (state == SERVICE_RUNNING) {
		status.dwControlsAccepted = SERVICE_ACCEPT_STOP;
	}
	status.dwWin32ExitCode = exit_code;
	status.dwServiceSpecificExitCode = service_exit_code;
	status.dwCheckPoint = check_point;
	status.dwWaitHint = wait_hint_ms;

	::SetServiceStatus(status_, &status);
}

void WindowsHIDService::status_ok(DWORD state) {
	if (state == SERVICE_RUNNING) {
		log(LogLevel::INFO, LogCategory::SERVICE, LogMessage::SVC_STARTED,
			0, ::gettext("Service started"));
	} else if (state == SERVICE_STOPPED) {
		log(LogLevel::INFO, LogCategory::SERVICE, LogMessage::SVC_STOPPED,
			0, ::gettext("Service stopped"));
	}
	report_status(state, NO_ERROR, 0, 0, 0);
}

void WindowsHIDService::status_pending(DWORD state, DWORD wait_hint_ms,
		DWORD check_point) {
	if (state == SERVICE_START_PENDING) {
		log(LogLevel::INFO, LogCategory::SERVICE, LogMessage::SVC_STARTING,
			0, ::gettext("Service starting"));
	} else if (state == SERVICE_STOP_PENDING) {
		log(LogLevel::INFO, LogCategory::SERVICE, LogMessage::SVC_STOPPING,
			0, ::gettext("Service stopping"));
	}
	report_status(state, NO_ERROR, 0, wait_hint_ms, check_point);
}

void WindowsHIDService::status_error(DWORD exit_code, DWORD service_exit_code) {
	log(LogLevel::ERROR, LogCategory::SERVICE, LogMessage::SVC_FAILED,
		0, ::gettext("Service failed"));
	report_status(SERVICE_STOPPED, exit_code, service_exit_code, 0, 0);
}

void WindowsHIDService::log(LogLevel level, LogCategory category,
		LogMessage message, int argc, const char *format...) {
	std::va_list argv;
	va_start(argv, format);
	win32::vlog(event_log_.get(), static_cast<WORD>(level),
		static_cast<WORD>(category), static_cast<DWORD>(message),
		nullptr, argc, format, argv, false);
	va_end(argv);
}

} // namespace hid_identify
