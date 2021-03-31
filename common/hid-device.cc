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
#include "hid-device.h"

#include <cstdint>
#include <string>

#include "types.h"
#include "usb-vid-pid.h"

namespace hid_identify {

static constexpr uint32_t RAW_USAGE_PAGE = 0xFF60;
static constexpr uint32_t RAW_USAGE_ID = 0x0061;
static constexpr uint32_t RAW_IN_USAGE_ID = 0x0062;
static constexpr uint32_t RAW_OUT_USAGE_ID = 0x0063;

void HIDDevice::open() {
	try {
		open(device_info_, reports_);
	} catch (...) {
		close();
		throw;
	}
}

void HIDDevice::identify() {
	open(device_info_, reports_);
	check_device_allowed();
	check_device_reports();
	send_report();
}

void HIDDevice::close() noexcept {
	device_info_ = {};
	reports_.clear();
	report_count_ = 0;
	clear();
}

void HIDDevice::check_device_allowed() {
	if (device_info_.interface == -1 || device_info_.interface == 1) {
		if (usb_device_allowed(device_info_.vendor, device_info_.product)) {
			return;
		}
	}

	log(LogLevel::ERROR, "Device not allowed");
	throw DisallowedUSBDevice{};
}

void HIDDevice::check_device_reports() {
	for (auto& report : reports_) {
		if (report.usage_page == RAW_USAGE_PAGE
				&& report.usage == RAW_USAGE_ID
				&& report.in.size() == 1
				&& report.out.size() == 1
				&& report.feature.empty()) {
			auto &in = report.in.front();
			auto &out = report.out.front();

			if (in.has_usage && in.usage == RAW_IN_USAGE_ID
					&& in.has_minimum && in.minimum == 0
					&& in.has_maximum && in.maximum == UINT8_MAX
					&& in.has_size && in.size == 8 /* bits */
					&& in.has_count && in.count > 0
					&& out.has_usage && out.usage == RAW_OUT_USAGE_ID
					&& out.has_minimum && out.minimum == 0
					&& out.has_maximum && out.maximum == UINT8_MAX
					&& out.has_size && out.size == 8 /* bits */
					&& out.has_count && out.count > 0) {
				report_count_ = out.count;
				return;
			}
		}
	}

	log(LogLevel::ERROR, "Not a QMK raw HID device interface");
	throw UnsupportedHIDReportUsage{};
}

void HIDDevice::send_report() {
	std::vector<uint8_t> data {
		/* Report ID */
		0x00,

		/* Command: Identify */
		0x00, 0x01,
	};

	/* OS */
	data.insert(data.end(), os_identity());

	if (report_count_ < data.size() - 1) {
		log(LogLevel::ERROR, "Report count too small for message (" + std::to_string(report_count_)
			+ " < " + std::to_string(data.size() - 1) + ")");
		throw IOLengthError{};
	}

	data.reserve(1 + report_count_);

	send_report(data);
	log(LogLevel::INFO, "Report sent");
}

} // namespace hid_identify
