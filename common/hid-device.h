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

#include <string>
#include <vector>

#include "types.h"

namespace hid_identify {

std::vector<uint8_t> os_identity();

class HIDDevice {
public:
	virtual ~HIDDevice() = default;

	void open();
	void identify();
	void close() noexcept;

	HIDDevice(const HIDDevice&) = delete;
	HIDDevice& operator=(const HIDDevice&) = delete;

protected:
	HIDDevice() = default;

	virtual void log(LogLevel level, LogCategory category, LogMessage message,
		int argc, const char *format...) noexcept = 0;

	virtual void open(USBDeviceInfo &device_info, std::vector<HIDReport> &reports) = 0;
	virtual void send_report(std::vector<uint8_t> &data) = 0;
	virtual void reset() noexcept;

private:
	void check_device_allowed();
	void check_device_reports();
	void send_report();

	USBDeviceInfo device_info_;
	std::vector<HIDReport> reports_;
	uint32_t report_count_ = 0;
};

} // namespace hid_identify
