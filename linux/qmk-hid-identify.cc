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

#include <iostream>
#include <string>
#include <vector>

#include "hid-report-desc.h"
#include "../common/usb-vid-pid.h"

#define RAW_USAGE_PAGE    0xFF60
#define RAW_USAGE_ID      0x0061
#define RAW_IN_USAGE_ID   0x0062
#define RAW_OUT_USAGE_ID  0x0063

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

QMKDevice::QMKDevice(const std::string &device) : device_(device) {
}

QMKDevice::~QMKDevice() {
	close();
}

int QMKDevice::open() {
	if (fd_ == -1) {
		fd_ = ::open(device_.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);

		if (fd_ == -1) {
			log(LOG_ERR, get_strerror());
			return EX_NOINPUT;
		}

		std::vector<char> buf(256);

		if (::ioctl(fd_, HIDIOCGRAWPHYS(buf.size()), buf.data()) < 0) {
			log(LOG_WARNING, "HIDIOCGRAWPHYS: " + get_strerror());
		} else {
			name_ = buf.data();
		}
	}

	return EX_OK;
}

int QMKDevice::identify() {
	open();

	int ret = is_allowed_device();

	if (ret != EX_OK) {
		if (ret == EX_UNAVAILABLE) {
			log(LOG_ERR, "Device not allowed");
		}

		return ret;
	}

	ret = is_qmk_device();

	if (ret != EX_OK) {
		if (ret == EX_UNAVAILABLE) {
			log(LOG_ERR, "Not a QMK raw HID device interface");
		} else if (ret == EX_DATAERR) {
			log(LOG_ERR, "Malformed report descriptor");
		}

		return ret;
	}

	return send_report();
}

void QMKDevice::close() {
	if (fd_ != -1) {
		::close(fd_);
		fd_ = -1;
		name_.clear();
	}
}

std::string QMKDevice::name() {
	return name_;
}

void QMKDevice::log(int level, const std::string &message) {
	std::string prefix = device_;

	if (!name_.empty()) {
		prefix += " (" + name_ + ")";
	}

	::syslog(LOG_USER | level, "%s: %s", prefix.c_str(), message.c_str());

	if (level < LOG_NOTICE) {
		std::cerr << prefix << ": " << message << std::endl;
	} else {
		std::cout << prefix << ": " << message << std::endl;
	}
}

int QMKDevice::is_allowed_device() {
	struct hidraw_devinfo info{};

	int ret = ::ioctl(fd_, HIDIOCGRAWINFO, &info);
	if (ret < 0) {
		log(LOG_ERR, "HIDIOCGRAWINFO: " + get_strerror());
		return EX_OSERR;
	}

	if (info.bustype == BUS_USB) {
		if (::usb_device_allowed(info.vendor, info.product)) {
			return EX_OK;
		}
	}

	return EX_UNAVAILABLE;
}

int QMKDevice::is_qmk_device() {
	struct hidraw_report_descriptor rpt_desc{};
	int desc_size = 0;

	int ret = ::ioctl(fd_, HIDIOCGRDESCSIZE, &desc_size);
	if (ret < 0) {
		log(LOG_ERR, "HIDIOCGRDESCSIZE: " + get_strerror());
		return EX_OSERR;
	}

	if (desc_size < 0) {
		log(LOG_ERR, "Report descriptor size is negative (" + std::to_string(desc_size) + ")");
		return EX_SOFTWARE;
	} else if ((unsigned int)desc_size > sizeof(rpt_desc.value)) {
		log(LOG_ERR, "Report descriptor size too large (" + std::to_string(desc_size)
			+ " > " + std::to_string(sizeof(rpt_desc.value)) + ")");
		return EX_SOFTWARE;
	}

	rpt_desc.size = desc_size;
	ret = ::ioctl(fd_, HIDIOCGRDESC, &rpt_desc);
	if (ret < 0) {
		log(LOG_ERR, "HIDIOCGRDESC: " + get_strerror());
		return EX_OSERR;
	}

	unsigned int pos = 0;
	do {
		struct hid_report hid_report{};

		ret = ::get_next_hid_usage(rpt_desc.value, rpt_desc.size, &pos, &hid_report);
		if (ret == 0) {
			if (hid_report.usage_page == RAW_USAGE_PAGE
					&& hid_report.usage == RAW_USAGE_ID
					&& hid_report.in.usage == RAW_IN_USAGE_ID
					&& hid_report.in.minimum == 0
					&& hid_report.in.maximum == UINT8_MAX
					&& hid_report.in.size == 8 /* bits */
					&& hid_report.in.count > 0
					&& hid_report.out.usage == RAW_OUT_USAGE_ID
					&& hid_report.out.minimum == 0
					&& hid_report.out.maximum == UINT8_MAX
					&& hid_report.out.size == 8 /* bits */
					&& hid_report.out.count > 0) {
				report_count_ = hid_report.out.count;
				return EX_OK;
			}
		} else if (ret == -1) {
			return EX_DATAERR;
		}
	} while (ret != 0);

	return EX_UNAVAILABLE;
}

int QMKDevice::send_report() {
	std::vector<uint8_t> data {
		/* Report ID */
		0x00,

		/* Command: Identify */
		0x00, 0x01,

		/* OS: Linux */
		'L', 'N', 'X', 0x00,
	};

	if (report_count_ < data.size() - 1) {
		log(LOG_ERR, "Report count too small for message (" + std::to_string(report_count_)
			+ " < " + std::to_string(data.size() - 1) + ")");
		return EX_IOERR;
	}

	data.reserve(1 + report_count_);

	ssize_t ret = ::write(fd_, data.data(), data.capacity());
	if (ret < 0 || (size_t)ret != data.capacity()) {
		log(LOG_ERR, "write: " + get_strerror());
		return EX_IOERR;
	}

	log(LOG_INFO, "Report sent");

	return EX_OK;
}
