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
#include <dbt.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include <deque>

#include "../common/types.h"
#include "windows++.h"

namespace hid_identify {

static const win32::native_string SVC_KEY = TEXT("qmk-hid-identify");
static const win32::native_string SVC_NAME = TEXT("QMK HID Identify");
static const win32::native_string SVC_DESC = TEXT("Identify the current OS to connected QMK HID devices");

int command_service();

class WindowsHIDService {
public:
	WindowsHIDService();

	void main();

private:
	DWORD run();
	DWORD startup();
	DWORD queue_all_devices();
	DWORD process_events();
	DWORD report_one_device();

	DWORD control(DWORD code, DWORD ev_type, LPVOID ev_data);
	void device_arrival(DEV_BROADCAST_DEVICEINTERFACE *dev_hdr);

	void report_status(DWORD state, DWORD exit_code, DWORD service_exit_code,
		DWORD wait_hint_ms, DWORD check_point);
	void status_ok(DWORD state);
	void status_pending(DWORD state, DWORD wait_hint_ms = 0,
		DWORD check_point = 0);
	void status_error(DWORD exit_code, DWORD service_exit_code = 0);

	void log(LogLevel level, LogCategory category, LogMessage message,
		int argc, const char *format...);

	win32::wrapped_ptr<HANDLE, ::DeregisterEventSource> event_log_;
	win32::wrapped_ptr<HANDLE, ::CloseHandle> stop_event_;
	win32::wrapped_ptr<HANDLE, ::CloseHandle> device_event_;
	win32::wrapped_ptr<HDEVNOTIFY, ::UnregisterDeviceNotification> device_notification_;
	SERVICE_STATUS_HANDLE status_;

	win32::wrapped_ptr<HANDLE, ::CloseHandle> devices_mutex_;
	std::deque<win32::native_string> devices_;
};

} // namespace hid_identify
