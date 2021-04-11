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
#include "service-control.h"

#include <windows.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include "events.h"
#include "registry.h"
#include "service.h"
#include "../common/types.h"
#include "windows++.h"

#include <array>
#include <iostream>
#include <vector>

namespace hid_identify {

static win32::sized_data<SERVICE_STATUS_PROCESS, DWORD> query_service_status(
		SC_HANDLE service) {
	auto status = win32::make_sized<SERVICE_STATUS_PROCESS, DWORD> (
		[&] (SERVICE_STATUS_PROCESS *data, DWORD size, DWORD *required_size) {
			DWORD tmp = 0;
			if (required_size == nullptr) {
				required_size = &tmp;
			}

			return QueryServiceStatusEx(service, SC_STATUS_PROCESS_INFO,
				reinterpret_cast<LPBYTE>(data), size, required_size);
		});
	if (!status) {
		auto error = ::GetLastError();
		win32::cerr << "QueryServiceStatusEx returned "
			<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
		throw OSError{};
	}
	return status;
}

static win32::sized_data<SERVICE_STATUS_PROCESS, DWORD> wait_service_status(
		SC_HANDLE service, DWORD pending_state) {
	auto status = query_service_status(service);
	DWORD start_time_ms = ::GetTickCount();
	DWORD check_point = status->dwCheckPoint;
	DWORD wait_hint_ms = status->dwWaitHint;

	if (status->dwCurrentState == pending_state) {
		if (pending_state == SERVICE_START_PENDING) {
			win32::cout << "Waiting for service to start" << std::endl;
		} else if (pending_state == SERVICE_STOP_PENDING) {
			win32::cout << "Waiting for service to stop" << std::endl;
		}
	}

	while (status->dwCurrentState == pending_state) {
		::Sleep(100);

		status = query_service_status(service);

		if (status->dwCheckPoint != check_point) {
			start_time_ms = ::GetTickCount();
			check_point = status->dwCheckPoint;
			wait_hint_ms = status->dwWaitHint;
		} else if (status->dwCurrentState != pending_state) {
			if (::GetTickCount() - start_time_ms > wait_hint_ms) {
				if (pending_state == SERVICE_START_PENDING) {
					win32::cerr << "Timeout starting service" << std::endl;
				} else if (pending_state == SERVICE_STOP_PENDING) {
					win32::cerr << "Timeout stopping service" << std::endl;
				}
				break;
			}
		}
	}

	return status;
}

void service_install() {
	auto filename = win32::current_process_filename();
	auto exec_command = TEXT("\"") + filename + TEXT("\" service");

	registry_add_event_log(false);

	::SetLastError(0);
	auto manager = win32::wrap_generic<SC_HANDLE, ::CloseServiceHandle>(
		::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!manager) {
		auto error = ::GetLastError();
		win32::cerr << "OpenSCManager returned "
			<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
		throw OSError{};
	}

	::SetLastError(0);
	auto service = win32::wrap_generic<SC_HANDLE, ::CloseServiceHandle>(
		::CreateService(
			manager.get(),
			SVC_KEY.c_str(),
			SVC_NAME.c_str(),
			SERVICE_ALL_ACCESS,
			SERVICE_WIN32_OWN_PROCESS,
			SERVICE_AUTO_START,
			SERVICE_ERROR_NORMAL,
			exec_command.c_str(),
			nullptr,
			nullptr,
			nullptr,
			nullptr,
			nullptr));
	if (!service) {
		auto error = ::GetLastError();
		if (error == ERROR_SERVICE_EXISTS) {
			win32::cout << "Service already exists" << std::endl;

			::SetLastError(0);
			service = win32::wrap_generic<SC_HANDLE, ::CloseServiceHandle>(
				::OpenService(
					manager.get(),
					SVC_KEY.c_str(),
					SERVICE_ALL_ACCESS));
			if (!service) {
				error = ::GetLastError();
				win32::cerr << "OpenService returned "
					<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
				throw OSError{};
			}

			::SetLastError(0);
			if (!::ChangeServiceConfig(
					service.get(),
					SERVICE_WIN32_OWN_PROCESS,
					SERVICE_AUTO_START,
					SERVICE_ERROR_NORMAL,
					exec_command.c_str(),
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					SVC_NAME.c_str())) {
				error = ::GetLastError();
				win32::cerr << "ChangeServiceConfig returned "
					<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
				throw OSError{};
			}

		} else {
			win32::cerr << "CreateService returned "
				<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
			throw OSError{};
		}
	} else {
		win32::cout << "Service installed" << std::endl;
	}

	::SetLastError(0);
	std::vector<win32::native_char> desc{SVC_DESC.c_str(), SVC_DESC.c_str() + SVC_DESC.length() + 1};
	SERVICE_DESCRIPTION svc_desc{desc.data()};
	if (!::ChangeServiceConfig2(service.get(), SERVICE_CONFIG_DESCRIPTION, &svc_desc)) {
		auto error = ::GetLastError();
		win32::cerr << "ChangeServiceConfig2 returned "
			<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
		throw OSError{};
	}

	::SetLastError(0);
	if (!::StartService(service.get(), 0, nullptr)) {
		auto error = ::GetLastError();
		if (error == ERROR_SERVICE_ALREADY_RUNNING) {
			win32::cout << "Service already running" << std::endl;
			return;
		} else {
			win32::cerr << "StartService returned "
				<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
			throw OSError{};
		}
	}

	auto status = wait_service_status(service.get(), SERVICE_START_PENDING);

	if (status->dwCurrentState != SERVICE_RUNNING) {
		win32::cerr << "Failed to start service" << std::endl;
	} else {
		win32::cout << "Service started" << std::endl;
	}
}

void service_uninstall() {
	::SetLastError(0);
	auto manager = win32::wrap_generic<SC_HANDLE, ::CloseServiceHandle>(
		::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
	if (!manager) {
		auto error = ::GetLastError();
		win32::cerr << "OpenSCManager returned "
			<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
		throw OSError{};
	}

	::SetLastError(0);
	auto service = win32::wrap_generic<SC_HANDLE, ::CloseServiceHandle>(
		::OpenService(
			manager.get(),
			SVC_KEY.c_str(),
			SERVICE_ALL_ACCESS));
	if (!service) {
		auto error = ::GetLastError();
		if (error == ERROR_SERVICE_DOES_NOT_EXIST) {
			win32::cout << "Service does not exist" << std::endl;
		} else {
			win32::cerr << "OpenService returned "
				<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
			throw OSError{};
		}
	} else {
		auto status = query_service_status(service.get());

		if (status->dwCurrentState != SERVICE_STOPPED) {
			if (status->dwCurrentState != SERVICE_STOP_PENDING) {
				SERVICE_STATUS control_status{};

				win32::cout << "Stopping service" << std::endl;

				::SetLastError(0);
				if (!::ControlService(service.get(), SERVICE_CONTROL_STOP, &control_status)) {
					auto error = ::GetLastError();
					win32::cerr << "ControlService returned "
						<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
					throw OSError{};
				}

				status = query_service_status(service.get());
			}

			status = wait_service_status(service.get(), SERVICE_STOP_PENDING);

			if (status->dwCurrentState != SERVICE_STOPPED) {
				win32::cerr << "Failed to stop service" << std::endl;
				return;
			} else {
				win32::cout << "Service stopped" << std::endl;
			}
		}

		::SetLastError(0);
		if (!::DeleteService(service.get())) {
			auto error = ::GetLastError();
			if (error == ERROR_SERVICE_MARKED_FOR_DELETE) {
				win32::cerr << "Service already marked for deletion" << std::endl;
				throw OSError{};
			} else {
				win32::cerr << "DeleteService returned "
					<< win32::ascii_to_native_string(win32::hex_error(error)) << std::endl;
				throw OSError{};
			}
		}

		win32::cout << "Service uninstalled" << std::endl;
	}
}

} // namespace hid_identify
