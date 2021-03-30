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
#include <sysexits.h>
#include <syslog.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

#include <initializer_list>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "../common/hid-device.h"
#include "../common/types.h"
#include "hid-report-desc.h"

namespace hid_identify {

/* POSIX */
static inline __attribute__((unused)) const char *call_strerror_r(std::vector<char> &buf, int (*func)(int, char *, size_t)) {
	if (func(errno, buf.data(), buf.size()) == 0) {
		return buf.data();
	} else {
		return nullptr;
	}
}

/* GNU */
static inline __attribute__((unused)) const char *call_strerror_r(std::vector<char> &buf, char *(*func)(int, char *, size_t)) {
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

int LinuxHIDDevice::open(USBDeviceInfo &device_info, std::vector<HIDReport> &reports) {
	if (!fd_) {
		auto fd = unique_fd(::open(pathname_.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC));
		if (!fd) {
			log(LogLevel::ERROR, get_strerror());
			return EX_NOINPUT;
		}

		int ret = init_device_info(fd.get(), device_info);
		if (ret != EX_OK) {
			return ret;
		}

		ret = init_reports(fd.get(), reports);
		if (ret != EX_OK) {
			return ret;
		}

		init_name(fd.get());

		fd_ = std::move(fd);
	}

	return EX_OK;
}

int LinuxHIDDevice::init_device_info(int fd, USBDeviceInfo &device_info) {
	struct hidraw_devinfo info{};

	if (::ioctl(fd, HIDIOCGRAWINFO, &info) < 0) {
		log(LogLevel::ERROR, "HIDIOCGRAWINFO: " + get_strerror());
		return EX_OSERR;
	}

	device_info = {
		.vendor = (uint16_t)info.vendor,
		.product = (uint16_t)info.product,
		.interface = -1
	};

	return EX_OK;
}

int LinuxHIDDevice::init_reports(int fd, std::vector<HIDReport> &reports) {
	struct hidraw_report_descriptor rpt_desc{};
	int desc_size = 0;

	if (::ioctl(fd, HIDIOCGRDESCSIZE, &desc_size) < 0) {
		log(LogLevel::ERROR, "HIDIOCGRDESCSIZE: " + get_strerror());
		return EX_OSERR;
	}

	if (desc_size < 0) {
		log(LogLevel::ERROR, "Report descriptor size is negative (" + std::to_string(desc_size) + ")");
		return EX_SOFTWARE;
	} else if ((unsigned int)desc_size > sizeof(rpt_desc.value)) {
		log(LogLevel::ERROR, "Report descriptor size too large (" + std::to_string(desc_size)
			+ " > " + std::to_string(sizeof(rpt_desc.value)) + ")");
		return EX_SOFTWARE;
	}

	rpt_desc.size = desc_size;
	if (::ioctl(fd, HIDIOCGRDESC, &rpt_desc) < 0) {
		log(LogLevel::ERROR, "HIDIOCGRDESC: " + get_strerror());
		return EX_OSERR;
	}

	unsigned int pos = 0;
	int ret;
	do {
		HIDReport hid_report{};

		ret = get_next_hid_usage(rpt_desc.value, rpt_desc.size, &pos, hid_report);
		if (ret == 0) {
			reports.emplace_back(hid_report);
		} else if (ret == -1) {
			return EX_DATAERR;
		}
	} while (ret != 0);

	return EX_OK;
}

void LinuxHIDDevice::init_name(int fd) {
	std::vector<char> buf(256);

	if (::ioctl(fd, HIDIOCGRAWPHYS(buf.size()), buf.data()) < 0) {
		log(LogLevel::WARNING, "HIDIOCGRAWPHYS: " + get_strerror());
		name_.clear();
	} else {
		name_ = buf.data();
	}
}

std::string LinuxHIDDevice::name() const {
	return name_;
}

void LinuxHIDDevice::close() {
	fd_.clear();
	name_.clear();
	HIDDevice::close();
}

void LinuxHIDDevice::log(LogLevel level, const std::string &message) {
	std::string prefix = pathname_;

	if (!name_.empty()) {
		prefix += " (" + name_ + ")";
	}

	int priority = LOG_USER;

	switch (level) {
	case LogLevel::ERROR:
		priority |= LOG_ERR;
		break;

	case LogLevel::WARNING:
		priority |= LOG_WARNING;
		break;

	case LogLevel::INFO:
		priority |= LOG_INFO;
		break;
	}

	::syslog(priority, "%s: %s", prefix.c_str(), message.c_str());

	if (level >= LogLevel::INFO) {
		std::cout << prefix << ": " << message << std::endl;
	} else {
		std::cerr << prefix << ": " << message << std::endl;
	}
}

int LinuxHIDDevice::send_report(std::vector<uint8_t> &data) {
	ssize_t ret = ::write(fd_.get(), data.data(), data.capacity());
	if (ret < 0 || (size_t)ret != data.capacity()) {
		log(LogLevel::ERROR, "write: " + get_strerror());
		return EX_IOERR;
	}

	return EX_OK;
}

} /* namespace hid_identify */
