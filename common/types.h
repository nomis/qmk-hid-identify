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
#pragma once

#include <cstdint>
#include <exception>
#include <vector>

#include "logging.h"

namespace hid_identify {

#ifdef LOGGING_HAS_LEVEL_IDS
#	define LOGGING_LEVEL(name) name = LOGGING_LEVEL_ ## name ## _ID
#else
#	define LOGGING_LEVEL(name) name
#endif

enum class LogLevel : unsigned short {
	LOGGING_LEVEL(ERROR),
	LOGGING_LEVEL(WARNING),
	LOGGING_LEVEL(INFO),
};

#undef LOGGING_LEVEL

#ifdef LOGGING_HAS_CATEGORY_IDS
#	define LOGGING_CATEGORY(name) name = LOGGING_CATEGORY_ ## name ## _ID
#else
#	define LOGGING_CATEGORY(name) name
#endif

enum class LogCategory : unsigned short {
	LOGGING_CATEGORY(REPORT_SENT),
	LOGGING_CATEGORY(OS_ERROR),
	LOGGING_CATEGORY(IO_ERROR),
	LOGGING_CATEGORY(UNSUPPORTED_DEVICE),
	LOGGING_CATEGORY(SERVICE),
};

#undef LOGGING_CATEGORY

#ifdef LOGGING_HAS_MESSAGE_IDS
#	define LOGGING_MESSAGE(name) name = LOGGING_MESSAGE_ ## name ## _ID
#else
#	define LOGGING_MESSAGE(name) name
#endif

enum class LogMessage : unsigned int {
	LOGGING_MESSAGE(DEV_REPORT_SENT),

	LOGGING_MESSAGE(DEV_NOT_ALLOWED),
	LOGGING_MESSAGE(DEV_UNKNOWN_USAGE),
	LOGGING_MESSAGE(DEV_UNKNOWN_USB_INTERFACE_NUMBER),
	LOGGING_MESSAGE(DEV_NO_HID_ATTRIBUTES),
	LOGGING_MESSAGE(DEV_ACCESS_DENIED),

	LOGGING_MESSAGE(DEV_REPORT_COUNT_TOO_SMALL),
	LOGGING_MESSAGE(DEV_REPORT_LENGTH_TOO_SMALL),

	LOGGING_MESSAGE(DEV_WRITE_FAILED),
	LOGGING_MESSAGE(DEV_WRITE_TIMEOUT),
	LOGGING_MESSAGE(DEV_SHORT_WRITE),

	LOGGING_MESSAGE(DEV_REPORT_DESCRIPTOR_SIZE_NEGATIVE),
	LOGGING_MESSAGE(DEV_REPORT_DESCRIPTOR_SIZE_TOO_LARGE),
	LOGGING_MESSAGE(DEV_MALFORMED_REPORT_DESCRIPTOR),

	LOGGING_MESSAGE(DEV_OS_FUNC_ERROR_CODE_1),
	LOGGING_MESSAGE(DEV_OS_FUNC_ERROR_CODE_2),
	LOGGING_MESSAGE(DEV_OS_FUNC_ERROR_PARAM_1_CODE_1),

	LOGGING_MESSAGE(SVC_STARTING),
	LOGGING_MESSAGE(SVC_STARTED),
	LOGGING_MESSAGE(SVC_STOPPING),
	LOGGING_MESSAGE(SVC_STOPPED),
	LOGGING_MESSAGE(SVC_FAILED),

	LOGGING_MESSAGE(SVC_MAIN_MUTEX_FAILURE),
	LOGGING_MESSAGE(SVC_CTRL_MUTEX_FAILURE),

	LOGGING_MESSAGE(SVC_POWER_RESUME),

	LOGGING_MESSAGE(SVC_OS_FUNC_ERROR_CODE_1),
	LOGGING_MESSAGE(SVC_OS_FUNC_ERROR_CODE_2),
};

#undef LOGGING_MESSAGE

struct USBDeviceInfo {
public:
	uint16_t vendor;
	uint16_t product;
	int16_t interface_number;
};

struct HIDCollection {
public:
	bool has_usage;
	uint32_t usage;
	bool has_minimum;
	uint32_t minimum;
	bool has_maximum;
	uint32_t maximum;
	bool has_count;
	uint32_t count;
	bool has_size;
	uint32_t size;
};

struct HIDReport {
public:
	uint32_t usage_page;
	uint32_t usage;

	std::vector<HIDCollection> in;
	std::vector<HIDCollection> out;
	std::vector<HIDCollection> feature;
};

class Exception: public std::exception {
public:
	virtual ~Exception() = default;

protected:
	Exception() noexcept = default;
};

class OSError: public Exception {
public:
	OSError() noexcept = default;
};

class OSLengthError: public OSError {
public:
	OSLengthError() noexcept = default;
};

class IOError: public Exception {
public:
	IOError() noexcept = default;
};

class IOLengthError: public IOError {
public:
	IOLengthError() noexcept = default;
};

class UnsupportedDevice: public Exception {
public:
	UnsupportedDevice() noexcept = default;
};

class UnavailableDevice: public UnsupportedDevice {
public:
	UnavailableDevice() noexcept = default;
};

class DisallowedUSBDevice: public UnsupportedDevice {
public:
	DisallowedUSBDevice() noexcept = default;
};

class UnsupportedHIDReportDescriptor: public UnsupportedDevice {
public:
	UnsupportedHIDReportDescriptor() noexcept = default;
};

class MalformedHIDReportDescriptor: public UnsupportedDevice {
public:
	MalformedHIDReportDescriptor() noexcept = default;
};

class UnsupportedHIDReportUsage: public UnsupportedDevice {
public:
	UnsupportedHIDReportUsage() noexcept = default;
};


} // namespace hid_identify
