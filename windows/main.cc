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
#include <setupapi.h>
#include <cfgmgr32.h>

extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}

#ifndef NOGDI
#	undef ERROR
#endif

#include "qmk-hid-identify.h"
#include "../common/types.h"
#include "windows++.h"

using namespace hid_identify;

static std::vector<win32::native_string> enumerate_devices() {
	std::vector<win32::native_string> devices;
	GUID guid{};
	::HidD_GetHidGuid(&guid);

	auto devinfo = win32::wrap_generic<HDEVINFO, ::SetupDiDestroyDeviceInfoList>(
		::SetupDiGetClassDevs(&guid, nullptr, nullptr, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT));

	SP_DEVICE_INTERFACE_DATA di_data;

	for (DWORD di_idx = 0;
			di_data = {}, di_data.cbSize = sizeof(di_data),
			::SetupDiEnumDeviceInterfaces(devinfo.get(), nullptr, &guid, di_idx, &di_data);
			di_idx++) {
		SP_DEVINFO_DATA do_data{};
		do_data.cbSize = sizeof(do_data);

		/* Get device filename */
		auto di_detail_data = win32::make_sized<SP_DEVICE_INTERFACE_DETAIL_DATA, DWORD>(
			[&] (SP_DEVICE_INTERFACE_DETAIL_DATA *data, DWORD size, DWORD *required_size) {
				if (data != nullptr) {
					data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
				}

				return ::SetupDiGetDeviceInterfaceDetail(devinfo.get(), &di_data,
					data, size, required_size, &do_data);
			}
		);
		if (!di_detail_data) {
			continue;
		}

		/* Check device is installed */
		auto install_state = win32::make_sized<BYTE, DWORD>(
			[&] (BYTE *data, DWORD size, DWORD *required_size) {
				return ::SetupDiGetDeviceRegistryProperty(devinfo.get(), &do_data,
					SPDRP_INSTALL_STATE, nullptr, data, size, required_size);
			}
		);
		if (!install_state) {
			continue;
		}

		if (install_state.size() != sizeof(DWORD)
				|| *(reinterpret_cast<DWORD*>(install_state.get())) != CM_INSTALL_STATE_INSTALLED) {
			continue;
		}

		devices.emplace_back(di_detail_data->DevicePath);
	}

	return devices;
}

int main(int, char *[]) {
	int exit_ret = 0;

	CM_WaitNoPendingInstallEvents(1000);

	try {
		for (auto& device : enumerate_devices()) {
			try {
				WindowsHIDDevice(device).identify();
			} catch (const Exception&) {
				exit_ret = exit_ret ? exit_ret : 1;
			}
		}
	} catch (...) {
		throw;
	}

	return exit_ret;
}
