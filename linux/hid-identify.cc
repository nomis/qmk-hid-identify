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
#include "hid-identify.h"

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

#include <cstdarg>
#include <iostream>
#include <string>
#include <utility>
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

std::vector<uint8_t> os_identity() {
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
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::DEV_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "open", get_strerror().c_str());
		throw UnavailableDevice{};
	}

	init_device_info(device_info);
	init_reports(reports);
	init_name();
}

void LinuxHIDDevice::init_device_info(USBDeviceInfo &device_info) {
	struct hidraw_devinfo info{};

	if (::ioctl(fd_.get(), HIDIOCGRAWINFO, &info) < 0) {
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::DEV_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "ioctl(HIDIOCGRAWINFO)", get_strerror().c_str());
		throw OSError{};
	}

	device_info = { (uint16_t)info.vendor, (uint16_t)info.product, -1 };
}

void LinuxHIDDevice::init_reports(std::vector<HIDReport> &reports) {
	struct hidraw_report_descriptor rpt_desc{};
	int desc_size = 0;

	if (::ioctl(fd_.get(), HIDIOCGRDESCSIZE, &desc_size) < 0) {
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::DEV_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "ioctl(HIDIOCGRDESCSIZE)", get_strerror().c_str());
		throw OSError{};
	}

	if (desc_size < 0) {
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::DEV_REPORT_DESCRIPTOR_SIZE_NEGATIVE,
			1, ::gettext("Report descriptor size is negative (%s)"),
			std::to_string(desc_size).c_str());
		throw OSLengthError{};
	} else if ((unsigned int)desc_size > sizeof(rpt_desc.value)) {
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::DEV_REPORT_DESCRIPTOR_SIZE_TOO_LARGE,
			2, ::gettext("Report descriptor size too large (%s > %s)"),
			std::to_string(desc_size).c_str(), std::to_string(sizeof(rpt_desc.value)).c_str());
		throw OSLengthError{};
	}

	rpt_desc.size = desc_size;
	if (::ioctl(fd_.get(), HIDIOCGRDESC, &rpt_desc) < 0) {
		log(LogLevel::ERROR, LogCategory::OS_ERROR, LogMessage::DEV_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "ioctl(HIDIOCGRDESC)", get_strerror().c_str());
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
			log(LogLevel::WARNING, LogCategory::UNSUPPORTED_DEVICE, LogMessage::DEV_MALFORMED_REPORT_DESCRIPTOR,
				0, ::gettext("Malformed report descriptor"));
			throw MalformedHIDReportDescriptor{};
		}
	} while (ret != 0);
}

void LinuxHIDDevice::init_name() {
	std::vector<char> buf(256);

	if (::ioctl(fd_.get(), HIDIOCGRAWPHYS(buf.size()), buf.data()) < 0) {
		log(LogLevel::WARNING, LogCategory::OS_ERROR, LogMessage::DEV_OS_FUNC_ERROR_CODE_1,
			2, ::gettext("%s: %s"), "ioctl(HIDIOCGRAWPHYS)", get_strerror().c_str());
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

void LinuxHIDDevice::log(LogLevel level,
		LogCategory category __attribute__((unused)),
		LogMessage message __attribute__((unused)),
		int argc __attribute__((unused)),
		const char *format...) noexcept {
	std::string prefix = pathname_;

	if (!name_.empty()) {
		prefix += " (" + name_ + ")";
	}

	std::vector<char> text(256);
	std::va_list args;

	va_start(args, format);
	if (std::vsnprintf(text.data(), text.size(), format, args) < 0) {
		text[0] = '?';
		text[1] = '\0';
	}
	va_end(args);

	::syslog(LOG_USER | static_cast<int>(level), "%s: %s", prefix.c_str(), text.data());

	if (level >= LogLevel::INFO) {
		std::cout << prefix << ": " << text.data() << std::endl;
	} else {
		std::cerr << prefix << ": " << text.data() << std::endl;
	}
}

void LinuxHIDDevice::send_report(std::vector<uint8_t> &data) {
	ssize_t ret = ::write(fd_.get(), data.data(), data.size());
	if (ret < 0) {
		log(LogLevel::ERROR, LogCategory::IO_ERROR, LogMessage::DEV_WRITE_FAILED,
			1, ::gettext("write: %s"), get_strerror().c_str());
		throw IOError{};
	} else if ((size_t)ret != data.size()) {
		log(LogLevel::ERROR, LogCategory::IO_ERROR, LogMessage::DEV_SHORT_WRITE,
			2, ::gettext("Write completed with only %s of %s bytes written"),
			std::to_string(ret).c_str(), std::to_string(data.size()).c_str());
		throw IOError{};
	}
}

} /* namespace hid_identify */
