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

#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

#include <initializer_list>
#include <iostream>
#include <string>
#include <vector>

#include "../common/hid-device.h"
#include "../common/types.h"
#include "hid-report-desc.h"

namespace hid_identify {

/* POSIX */
static inline __attribute__((unused)) const char *call_strerror_r(
		std::vector<char> &buf, int (*func)(int, char *, size_t)) {
	if (func(errno, buf.data(), buf.size()) == 0) {
		return buf.data();
	} else {
		return nullptr;
	}
}

/* GNU */
static inline __attribute__((unused)) const char *call_strerror_r(
		std::vector<char> &buf, char *(*func)(int, char *, size_t)) {
	return func(errno, buf.data(), buf.size());
}

static std::string get_strerror() {
	std::vector<char> buf(1024);

	auto ret = call_strerror_r(buf, ::strerror_r);
	if (ret != nullptr) {
		return ret;
	} else {
		return std::to_string(errno);
	}
}

std::initializer_list<uint8_t> os_identity() {
	return {'L', 'N', 'X', 0};
}

LinuxHIDDevice::LinuxHIDDevice(const std::string &pathname) : pathname_(pathname) {
}

void LinuxHIDDevice::open(USBDeviceInfo &device_info, std::vector<HIDReport> &reports) {
	if (fd_) {
		return;
	}

	fd_ = unique_fd(::open(pathname_.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC));
	if (!fd_) {
		log(LogLevel::Error, get_strerror());
		throw UnavailableDevice{};
	}

	init_device_info(device_info);
	init_reports(reports);
	init_name();
}

void LinuxHIDDevice::init_device_info(USBDeviceInfo &device_info) {
	struct hidraw_devinfo info{};

	if (::ioctl(fd_.get(), HIDIOCGRAWINFO, &info) < 0) {
		log(LogLevel::Error, "HIDIOCGRAWINFO: " + get_strerror());
		throw OSError{};
	}

	device_info = { (uint16_t)info.vendor, (uint16_t)info.product, -1 };
}

void LinuxHIDDevice::init_reports(std::vector<HIDReport> &reports) {
	struct hidraw_report_descriptor rpt_desc{};
	int desc_size = 0;

	if (::ioctl(fd_.get(), HIDIOCGRDESCSIZE, &desc_size) < 0) {
		log(LogLevel::Error, "HIDIOCGRDESCSIZE: " + get_strerror());
		throw OSError{};
	}

	if (desc_size < 0) {
		log(LogLevel::Error, "Report descriptor size is negative (" + std::to_string(desc_size) + ")");
		throw OSLengthError{};
	} else if ((unsigned int)desc_size > sizeof(rpt_desc.value)) {
		log(LogLevel::Error, "Report descriptor size too large (" + std::to_string(desc_size)
			+ " > " + std::to_string(sizeof(rpt_desc.value)) + ")");
		throw OSLengthError{};
	}

	rpt_desc.size = desc_size;
	if (::ioctl(fd_.get(), HIDIOCGRDESC, &rpt_desc) < 0) {
		log(LogLevel::Error, "HIDIOCGRDESC: " + get_strerror());
		throw OSError{};
	}

	unsigned int pos = 0;
	int ret;
	do {
		HIDReport hid_report{};

		ret = get_next_hid_usage(rpt_desc.value, rpt_desc.size, &pos, hid_report);
		if (ret == 0) {
			reports.emplace_back(std::move(hid_report));
		} else if (ret == -1) {
			log(LogLevel::Error, "Malformed report descriptor");
			throw MalformedHIDReportDescriptor{};
		}
	} while (ret != 0);
}

void LinuxHIDDevice::init_name() {
	std::vector<char> buf(256);

	if (::ioctl(fd_.get(), HIDIOCGRAWPHYS(buf.size()), buf.data()) < 0) {
		log(LogLevel::Warning, "HIDIOCGRAWPHYS: " + get_strerror());
		name_.clear();
	} else {
		name_ = buf.data();
	}
}

void LinuxHIDDevice::reset() noexcept {
	fd_.clear();
	name_.clear();
	report_count_ = 0;
}

void LinuxHIDDevice::log(LogLevel level, const std::string &message) {
	std::string prefix = pathname_;

	if (!name_.empty()) {
		prefix += " (" + name_ + ")";
	}

	int priority = LOG_USER;

	switch (level) {
	case LogLevel::Error:
		priority |= LOG_ERR;
		break;

	case LogLevel::Warning:
		priority |= LOG_WARNING;
		break;

	case LogLevel::Info:
		priority |= LOG_INFO;
		break;
	}

	::syslog(priority, "%s: %s", prefix.c_str(), message.c_str());

	if (level >= LogLevel::Info) {
		std::cout << prefix << ": " << message << std::endl;
	} else {
		std::cerr << prefix << ": " << message << std::endl;
	}
}

void LinuxHIDDevice::send_report(std::vector<uint8_t> &data) {
	ssize_t ret = ::write(fd_.get(), data.data(), data.size());
	if (ret < 0 || (size_t)ret != data.size()) {
		log(LogLevel::Error, "write: " + get_strerror());
		throw IOError{};
	}
}

} /* namespace hid_identify */
