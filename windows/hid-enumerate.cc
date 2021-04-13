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
#include "hid-enumerate.h"

#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>

extern "C" {
#include <hidsdi.h>
}

#ifndef NOGDI
#	undef ERROR
#endif

#include <iostream>
#include <string>

#include "windows++.h"

namespace hid_identify {

WindowsHIDEnumeration::const_iterator::const_iterator(
		const WindowsHIDEnumeration &di_shared, DWORD di_idx)
		: di_shared_(di_shared), di_idx_(di_idx) {
	populate_device_path();
}

WindowsHIDEnumeration::const_iterator& WindowsHIDEnumeration::const_iterator::operator++() {
	di_idx_++;
	populate_device_path();
	return *this;
}

WindowsHIDEnumeration::const_iterator WindowsHIDEnumeration::const_iterator::operator++(int) {
	const_iterator ret = *this;
	++(*this);
	return ret;
}

bool WindowsHIDEnumeration::const_iterator::operator==(const WindowsHIDEnumeration::const_iterator &other) const {
	return di_idx_ == other.di_idx_;
}

bool WindowsHIDEnumeration::const_iterator::operator!=(const WindowsHIDEnumeration::const_iterator &other) const {
	return !(*this == other);
}

const WindowsHIDEnumeration::const_iterator::reference WindowsHIDEnumeration::const_iterator::operator*() const {
	return device_path_;
}

void WindowsHIDEnumeration::const_iterator::populate_device_path() {
	for (; di_idx_ != MAXDWORD; di_idx_++) {
		SP_DEVICE_INTERFACE_DATA di_data{};
		di_data.cbSize = sizeof(di_data);

		if (!::SetupDiEnumDeviceInterfaces(di_shared_.devinfo_.get(), nullptr,
				&di_shared_.guid_, di_idx_, &di_data)) {
			di_idx_ = MAXDWORD;
			break;
		}

		SP_DEVINFO_DATA do_data{};
		do_data.cbSize = sizeof(do_data);

		/* Get device filename */
		auto di_detail_data = win32::make_sized<SP_DEVICE_INTERFACE_DETAIL_DATA, DWORD>(
			[&] (SP_DEVICE_INTERFACE_DETAIL_DATA *data, DWORD size, DWORD *required_size) {
				if (data != nullptr) {
					data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
				}

				return ::SetupDiGetDeviceInterfaceDetail(di_shared_.devinfo_.get(),
					&di_data, data, size, required_size, &do_data);
			}
		);
		if (!di_detail_data) {
			continue;
		}

		/* Check device is installed */
		auto install_state = win32::make_sized<BYTE, DWORD>(
			[&] (BYTE *data, DWORD size, DWORD *required_size) {
				return ::SetupDiGetDeviceRegistryProperty(di_shared_.devinfo_.get(),
					&do_data, SPDRP_INSTALL_STATE, nullptr, data, size, required_size);
			}
		);
		if (!install_state) {
			continue;
		}

		if (install_state.size() != sizeof(DWORD)
				|| *(reinterpret_cast<DWORD*>(install_state.get())) != CM_INSTALL_STATE_INSTALLED) {
			continue;
		}

		device_path_ = di_detail_data->DevicePath;
		return;
	}

	if (di_idx_ == MAXDWORD) {
		device_path_ = L"";
	}
}

WindowsHIDEnumeration::WindowsHIDEnumeration() {
	::HidD_GetHidGuid(&guid_);

	::CM_WaitNoPendingInstallEvents(1000);

	::SetLastError(0);
	devinfo_ = win32::wrap_valid_handle<HDEVINFO, ::SetupDiDestroyDeviceInfoList>(
		::SetupDiGetClassDevs(&guid_, nullptr, nullptr,
			DIGCF_DEVICEINTERFACE | DIGCF_PRESENT));
	if (!devinfo_) {
		throw win32::Exception1{"SetupDiGetClassDevs"};
	}
}

WindowsHIDEnumeration::const_iterator WindowsHIDEnumeration::begin() const {
	return cbegin();
}

WindowsHIDEnumeration::const_iterator WindowsHIDEnumeration::end() const {
	return cend();
}

WindowsHIDEnumeration::const_iterator WindowsHIDEnumeration::cbegin() const {
	return WindowsHIDEnumeration::const_iterator(*this, 0);
}

WindowsHIDEnumeration::const_iterator WindowsHIDEnumeration::cend() const {
	return WindowsHIDEnumeration::const_iterator(*this, MAXDWORD);
}

} // namespace hid_identify
