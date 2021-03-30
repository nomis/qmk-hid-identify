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

#include <cstdint>
#include <string>
#include <vector>

#include "../common/hid-device.h"
#include "../common/types.h"
#include "unique-fd.h"

namespace hid_identify {

class LinuxHIDDevice: public HIDDevice {
public:
	LinuxHIDDevice(const std::string &pathname);

	int open(USBDeviceInfo &device_info, std::vector<HIDReport> &reports) override;
	std::string name() const;
	void close() override;

	void log(LogLevel level, const std::string &message) override;

protected:
	int send_report(std::vector<uint8_t> &data) override;

private:
	int init_device_info(int fd, USBDeviceInfo &device_info);
	int init_reports(int fd, std::vector<HIDReport> &reports);
	void init_name(int fd);

	const std::string pathname_;
	unique_fd fd_;
	std::string name_;
	uint32_t report_count_ = 0;
};

} // namespace hid_identify
