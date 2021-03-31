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
#include "qmk-hid-identify.h"

#include <windows.h>

extern "C" {
#include <hidsdi.h>
#include <hidpi.h>
}

#include <algorithm>
#include <cctype>
#include <cwctype>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

#include "../common/hid-device.h"
#include "../common/types.h"
#include "windows++.h"

namespace hid_identify {

std::initializer_list<uint8_t> os_identity() {
	return {'W', 'I', 'N', 0};
}

WindowsHIDDevice::WindowsHIDDevice(const win32::native_string &filename)
		: filename_(filename) {
}

void WindowsHIDDevice::open(USBDeviceInfo &device_info, std::vector<HIDReport> &reports) {
	if (handle_) {
		return;
	}

	::SetLastError(0);
	handle_ = win32::wrap_handle(::CreateFile(filename_.c_str(), GENERIC_WRITE,
			FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr));
	if (!handle_) {
		log(LogLevel::Error, "CreateFile returned " + win32::last_error());
		throw UnavailableDevice{};
	}

	init_device_info(device_info);
	init_reports(reports);
}

int16_t WindowsHIDDevice::interface_number() {
	static const win32::native_string tag_uc = TEXT("&MI_");
	static const win32::native_string tag_lc = TEXT("&mi_");

	auto pos = filename_.find(tag_uc);
	if (pos == win32::native_string::npos) {
		pos = filename_.find(tag_lc);
		if (pos == win32::native_string::npos) {
			return -1;
		}
	}

	pos += tag_uc.length();
	if (filename_.length() - pos < 2) {
		return -1;
	}

	auto hi = filename_[pos];
	auto lo = filename_[pos + 1];
	if (!win32::isxdigit(hi) || !win32::isxdigit(lo)) {
		return -1;
	}

	return std::stoi(win32::native_string{{hi, lo}}, nullptr, 16);
}

void WindowsHIDDevice::init_device_info(USBDeviceInfo &device_info) {
	HIDD_ATTRIBUTES attrs{};
	attrs.Size = sizeof(HIDD_ATTRIBUTES);

	if (!::HidD_GetAttributes(handle_.get(), &attrs)) {
		log(LogLevel::Error, "Unable to get HID attributes");
		throw OSError{};
	}

	device_info = { attrs.VendorID, attrs.ProductID, interface_number() };

	// This should always be known
	if (device_info.interface_number == -1) {
		throw DisallowedUSBDevice{};
	}
}

std::vector<HIDCollection> WindowsHIDDevice::caps_to_collections(
		USAGE usage_page, HIDP_REPORT_TYPE report_type, USHORT len,
		PHIDP_PREPARSED_DATA preparsed_data) {
	std::vector<HIDP_VALUE_CAPS> vcaps(len);

	NTSTATUS ret = ::HidP_GetSpecificValueCaps(report_type, usage_page,
			0, 0, vcaps.data(), &len, preparsed_data);
	if (ret == HIDP_STATUS_USAGE_NOT_FOUND) {
		return {};
	} else if (ret != HIDP_STATUS_SUCCESS) {
		log(LogLevel::Error, "HidP_GetSpecificValueCaps failed for report type "
			+ std::to_string(report_type) + ": " + win32::hex_error(ret));
		throw OSError{};
	}

	std::vector<HIDCollection> collections;

	for (size_t i = 0; i < len; i++) {
		HIDCollection collection{};

		if (vcaps[i].IsRange) {
			throw UnsupportedHIDReportDescriptor{};
		}

		collection.has_usage = true;
		collection.usage = vcaps[i].NotRange.Usage;

		collection.has_minimum = true;
		collection.minimum = vcaps[i].LogicalMin;

		collection.has_maximum = true;
		collection.maximum = vcaps[i].LogicalMax;

		collection.has_count = true;
		collection.count = vcaps[i].ReportCount;

		collection.has_size = true;
		collection.size = vcaps[i].BitSize;

		collections.emplace_back(std::move(collection));
	}

	return collections;
}

void WindowsHIDDevice::init_reports(std::vector<HIDReport> &reports) {
	::SetLastError(0);
	auto preparsed_data = win32::wrap_output<PHIDP_PREPARSED_DATA, ::HidD_FreePreparsedData>(
		[&] (PHIDP_PREPARSED_DATA &data) {
			return ::HidD_GetPreparsedData(handle_.get(), &data);
		});
	if (!preparsed_data) {
		log(LogLevel::Error, "HidD_GetPreparsedData returned " + win32::last_error());
		throw OSError{};
	}

	HIDP_CAPS caps{};
	NTSTATUS ret = ::HidP_GetCaps(preparsed_data.get(), &caps);
	if (ret != HIDP_STATUS_SUCCESS) {
		log(LogLevel::Error, "HidP_GetCaps failed: " + win32::hex_error(ret));
		throw OSError{};
	}

	report_length_ = caps.OutputReportByteLength;

	HIDReport report{};

	report.usage_page = caps.UsagePage;
	report.usage = caps.Usage;

	report.in = caps_to_collections(caps.UsagePage, HidP_Input,
		caps.NumberInputValueCaps, preparsed_data.get());
	report.out = caps_to_collections(caps.UsagePage, HidP_Output,
		caps.NumberOutputValueCaps, preparsed_data.get());
	report.feature = caps_to_collections(caps.UsagePage, HidP_Feature,
		caps.NumberFeatureValueCaps, preparsed_data.get());

	reports.emplace_back(std::move(report));
}

void WindowsHIDDevice::reset() noexcept {
	handle_.reset();
}

void WindowsHIDDevice::log(LogLevel level, const std::string &message) {
	if (level >= LogLevel::Info) {
		win32::cout << filename_ << ": " << win32::ascii_to_native_string(message) << std::endl;
	} else {
		win32::cerr << filename_ << ": " << win32::ascii_to_native_string(message) << std::endl;
	}
}

void WindowsHIDDevice::send_report(std::vector<uint8_t> &data) {
	// Minimum length is OutputReportByteLength (which includes the Report ID)
	if (report_length_ < data.size()) {
		log(LogLevel::Error, "Report length too small for message (" + std::to_string(report_length_)
			+ " < " + std::to_string(data.size()) + ")");
		throw IOLengthError{};
	}

	data.resize(report_length_);

	::SetLastError(0);
	auto event = win32::wrap_handle(::CreateEvent(nullptr, true, false, nullptr));
	if (!event) {
		log(LogLevel::Error, "CreateEvent returned " + win32::last_error());
		throw OSError{};
	}

	// Must cancel and wait for finish of I/O operation before freeing this
	OVERLAPPED overlapped{};
	overlapped.hEvent = event.get();

	::SetLastError(0);
	if (!::WriteFile(handle_.get(), data.data(), data.size(), nullptr, &overlapped)) {
		if (::GetLastError() != ERROR_IO_PENDING) {
			log(LogLevel::Error, "WriteFile returned " + win32::last_error());
			throw OSError{};
		}
	}

	::SetLastError(0);
	DWORD res = ::WaitForSingleObject(event.get(), 1000);
	if (res != WAIT_OBJECT_0) {
		log(LogLevel::Warning, "WaitForSingleObject returned "
			+ std::to_string(res) + ", " + win32::last_error());

		::CancelIo(handle_.get());
	}

	DWORD written = 0;
	::SetLastError(0);
	if (::GetOverlappedResult(handle_.get(), &overlapped, &written, true)) {
		if (written != data.size()) {
			log(LogLevel::Error, "Write completed with only " + std::to_string(written)
				+ " of " + std::to_string(data.size()) + " bytes");
			throw IOError{};
		}
	} else {
		log(LogLevel::Error, "GetOverlappedResult returned " + win32::last_error());
		throw IOError{};
	}
}

} /* namespace hid_identify */
