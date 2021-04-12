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

extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}

#ifndef NOGDI
#	undef ERROR
#endif

#include <cstdint>
#include <string>
#include <vector>

#include "../common/hid-device.h"
#include "../common/types.h"
#include "windows++.h"

namespace hid_identify {

static const std::wstring LOG_PROVIDER = L"uk.uuid-QMK HID Identify";

class WindowsHIDDevice: public HIDDevice {
public:
	explicit WindowsHIDDevice(const std::wstring &filename);

protected:
	void log(LogLevel level, LogCategory category, LogMessage message,
		int argc, const char *format...) noexcept override;

	void open(USBDeviceInfo &device_info, std::vector<HIDReport> &reports) override;
	void send_report(std::vector<uint8_t> &data) override;
	void reset() noexcept override;

private:
	int16_t interface_number();
	void init_device_info(USBDeviceInfo &device_info);
	std::vector<HIDCollection> caps_to_collections(
		USAGE usage_page, HIDP_REPORT_TYPE report_type, USHORT len,
		PHIDP_PREPARSED_DATA preparsed_data);
	void init_reports(std::vector<HIDReport> &reports);

	const std::wstring filename_;
	win32::wrapped_ptr<HANDLE, ::DeregisterEventSource> event_log_;
	win32::wrapped_ptr<HANDLE, ::CloseHandle> handle_;
	uint32_t report_length_ = 0;
};

} // namespace hid_identify
