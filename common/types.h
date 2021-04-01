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
#include <exception>
#include <vector>

namespace hid_identify {

enum class LogLevel {
	ERROR,
	WARNING,
	INFO,
};

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
