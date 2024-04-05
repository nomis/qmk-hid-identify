/*
	qmk-hid-identify - Identify the current OS to QMK device
	Copyright 2021-2022  Simon Arlott

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
#include <dbt.h>

extern "C" {
#include <hidsdi.h>
}

#ifndef NOGDI
#	undef ERROR
#endif

#include "events.h"
#include "hid-enumerate.h"
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
	std::vector<wchar_t> name{SVC_KEY.c_str(), SVC_KEY.c_str() + SVC_KEY.length() + 1};
	std::array<SERVICE_TABLE_ENTRY, 2> dispatch_table{
		{
			{ name.data(), [] __stdcall (DWORD, LPTSTR[]) noexcept {
					static WindowsHIDService service;
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
		[] __stdcall (DWORD code, DWORD ev_type, LPVOID ev_data, LPVOID context) noexcept -> DWORD {
			WindowsHIDService *service = static_cast<WindowsHIDService*>(context);
			try {
				return service->control(code, ev_type, ev_data);
			} catch (const Exception &e) {
				return service->control(SERVICE_CONTROL_STOP, 0, 0);
			} catch (const win32::Exception1 &e) {
				service->log(e);
				return service->control(SERVICE_CONTROL_STOP, 0, 0);
			} catch (const win32::Exception2 &e) {
				service->log(e);
				return service->control(SERVICE_CONTROL_STOP, 0, 0);
			}
		}, this);
	if (status_ == 0) {
		auto error = ::GetLastError();
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "RegisterServiceCtrlHandlerEx", win32::hex_error(error).c_str());
		throw OSError{};
	}

	try {
		try {
			DWORD exit_code = run();
			if (exit_code == NO_ERROR) {
				status_ok(SERVICE_STOPPED);
			} else {
				status_error(exit_code);
			}
		} catch (const win32::Exception1 &e) {
			log(e);
			throw;
		} catch (const win32::Exception2 &e) {
			log(e);
			throw;
		}
	} catch (const win32::Exception &e) {
		status_error(e.error());
		throw;
	} catch (...) {
		status_error(ERROR_SERVICE_SPECIFIC_ERROR, 1);
		throw;
	}
}

DWORD WindowsHIDService::run() {
	DWORD ret;

	status_pending(SERVICE_START_PENDING, 5000, 1);

	ret = startup();
	if (ret != NO_ERROR) {
		return ret;
	}

	status_ok(SERVICE_RUNNING);

	ret = queue_all_devices();
	if (ret != NO_ERROR) {
		return ret;
	}

	return process_events();
}

DWORD WindowsHIDService::startup() {
	::SetLastError(0);
	stop_event_ = win32::wrap_generic_handle(::CreateEvent(nullptr, true, false, nullptr));
	if (!stop_event_) {
		throw win32::Exception1{"CreateEvent"};
	}

	::SetLastError(0);
	power_resume_event_ = win32::wrap_generic_handle(::CreateEvent(nullptr, false, false, nullptr));
	if (!power_resume_event_) {
		throw win32::Exception1{"CreateEvent"};
	}

	::SetLastError(0);
	device_event_ = win32::wrap_generic_handle(::CreateEvent(nullptr, false, false, nullptr));
	if (!device_event_) {
		throw win32::Exception1{"CreateEvent"};
	}

	::SetLastError(0);
	devices_mutex_ = win32::wrap_generic_handle(::CreateMutex(nullptr, false, nullptr));
	if (!device_event_) {
		throw win32::Exception1{"CreateMutex"};
	}

	DEV_BROADCAST_DEVICEINTERFACE filter{};
	filter.dbcc_size = sizeof(filter);
	filter.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
	::HidD_GetHidGuid(&filter.dbcc_classguid);

	::SetLastError(0);
	device_notification_ = win32::wrap_generic<HDEVNOTIFY, ::UnregisterDeviceNotification>(
		::RegisterDeviceNotification(status_, &filter, DEVICE_NOTIFY_SERVICE_HANDLE));
	if (!device_notification_) {
		throw win32::Exception1{"RegisterDeviceNotification"};
	}

	return NO_ERROR;
}

DWORD WindowsHIDService::queue_all_devices() {
	for (const auto& device : WindowsHIDEnumeration()) {
		::SetLastError(0);
		DWORD ret = ::WaitForSingleObject(stop_event_.get(), 0);
		if (ret == WAIT_OBJECT_0) {
			return NO_ERROR;
		} else if (ret != WAIT_TIMEOUT) {
			throw win32::Exception2{"WaitForSingleObject(stop_event)", ret};
		}

		auto lock = win32::acquire_mutex(devices_mutex_.get());
		if (!lock) {
			log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_MAIN_MUTEX_FAILURE,
				0, ::gettext("Service main thread failed to acquire device queue mutex"));
			return ERROR_SERVICE_SPECIFIC_ERROR;
		}
		devices_.emplace_back(std::move(device));
	}

	return NO_ERROR;
}

DWORD WindowsHIDService::process_events() {
	const std::array<HANDLE, 3> wait_handles{
		{
			stop_event_.get(),
			power_resume_event_.get(),
			device_event_.get()
		}
	};

	while (1) {
		DWORD ret = report_one_device();
		DWORD wait_ms;

		if (ret == NO_ERROR) {
			// check for service stop between every device
			wait_ms = 0;
		} else if (ret == ERROR_NO_MORE_FILES) {
			wait_ms = INFINITE;
		} else if (ret != NO_ERROR) {
			return ret;
		}

		::SetLastError(0);
		ret = ::WaitForMultipleObjects(wait_handles.size(),
			wait_handles.data(), FALSE, wait_ms);
		if (ret == WAIT_OBJECT_0) { // stop_event_
			return NO_ERROR;
		} else if (ret == WAIT_OBJECT_0 + 1) { // power_resume_event_
			queue_all_devices();
		} else if (ret == WAIT_OBJECT_0 + 2) { // device_event_
			// continue
		} else if (ret != WAIT_TIMEOUT) {
			throw win32::Exception2{"WaitForMultipleObjects({stop_event,device_event})", ret};
		}
	}
}

DWORD WindowsHIDService::report_one_device() {
	auto lock = win32::acquire_mutex(devices_mutex_.get());
	if (!lock) {
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_MAIN_MUTEX_FAILURE,
			0, ::gettext("Service main thread failed to acquire device queue mutex"));
		return ERROR_SERVICE_SPECIFIC_ERROR;
	}

	bool empty = true;
	if (!devices_.empty()) {
		auto device = devices_.front();
		devices_.pop_front();
		empty = devices_.empty();
		if (empty) {
			devices_.shrink_to_fit();
		}
		lock.reset();

		try {
			WindowsHIDDevice(device).identify();
		} catch (const Exception&) {
			// ignored
		}
	}

	if (empty) {
		return ERROR_NO_MORE_FILES;
	} else {
		return NO_ERROR;
	}
}

DWORD WindowsHIDService::control(DWORD code, DWORD ev_type, LPVOID ev_data) {
	switch (code) {
		case SERVICE_CONTROL_INTERROGATE:
			return NO_ERROR;

		case SERVICE_CONTROL_STOP:
			status_pending(SERVICE_STOP_PENDING, 5000, 1);
			::SetEvent(stop_event_.get());
			return NO_ERROR;

		case SERVICE_CONTROL_DEVICEEVENT:
			if (ev_type == DBT_DEVICEARRIVAL) {
				device_arrival(static_cast<DEV_BROADCAST_DEVICEINTERFACE*>(ev_data));
			}
			return NO_ERROR;

		case SERVICE_CONTROL_POWEREVENT:
			if (ev_type == PBT_APMRESUMEAUTOMATIC) {
				log(LogLevel::INFO, LogCategory::SERVICE, LogMessage::SVC_POWER_RESUME,
					0, ::gettext("Power resumed"));
				::SetEvent(power_resume_event_.get());
			}
			return NO_ERROR;

		default:
			return ERROR_CALL_NOT_IMPLEMENTED;
	}
}

void WindowsHIDService::device_arrival(DEV_BROADCAST_DEVICEINTERFACE *dev) {
	if (dev == nullptr) {
		return;
	}

	if (dev->dbcc_devicetype != DBT_DEVTYP_DEVICEINTERFACE) {
		return;
	}

	auto lock = win32::acquire_mutex(devices_mutex_.get());
	if (!lock) {
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_CTRL_MUTEX_FAILURE,
			0, ::gettext("Service control handler failed to acquire device queue mutex"));
		control(SERVICE_CONTROL_STOP, 0, 0);
		return;
	}
	devices_.emplace_back(dev->dbcc_name);
	::SetEvent(device_event_.get());
}

void WindowsHIDService::report_status(DWORD state, DWORD exit_code,
		DWORD service_exit_code, DWORD wait_hint_ms, DWORD check_point) {
	SERVICE_STATUS status{};

	status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	status.dwCurrentState = state;
	if (state == SERVICE_RUNNING) {
		status.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_POWEREVENT;
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
		LogMessage message, int argc, const char *format...) noexcept {
	std::va_list argv;
	va_start(argv, format);
	win32::vlog(event_log_.get(), static_cast<WORD>(level),
		static_cast<WORD>(category), static_cast<DWORD>(message),
		nullptr, argc, format, argv, false);
	va_end(argv);
}

void WindowsHIDService::log(const win32::Exception1 &e) noexcept {
	log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_OS_FUNC_ERROR_CODE_1,
		2, ::gettext("%s: %s"), e.function_name().c_str(),
		win32::hex_error(e.error()).c_str());
}

void WindowsHIDService::log(const win32::Exception2 &e) noexcept {
	log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::SVC_OS_FUNC_ERROR_CODE_2,
		3, ::gettext("%s: %s, %s"), e.function_name().c_str(),
		win32::hex_error(e.return_code()).c_str(),
		win32::hex_error(e.error()).c_str());
}

} // namespace hid_identify
