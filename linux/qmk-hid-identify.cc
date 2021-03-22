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
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sysexits.h>
#include <unistd.h>

#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include "qmk-hid-identify.h"
#include "hid-report-desc.h"

#define RAW_USAGE_PAGE 0xFF60
#define RAW_USAGE_ID 0x61

static int is_qmk_device(int fd) {
	struct hidraw_report_descriptor rpt_desc{};
	int desc_size = 0;

	int ret = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
	if (ret < 0) {
		perror("HIDIOCGRDESCSIZE");
		return EX_OSERR;
	}

	if (desc_size < 0) {
		std::cerr << "Report descriptor size is negative (" << desc_size << ")" << std::endl;
		return EX_SOFTWARE;
	} else if ((unsigned int)desc_size > sizeof(rpt_desc.value)) {
		std::cerr << "Report descriptor size too large (" << desc_size << " > " << sizeof(rpt_desc.value) << ")" << std::endl;
		return EX_SOFTWARE;
	}

	rpt_desc.size = desc_size;
	ret = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
	if (ret < 0) {
		perror("HIDIOCGRDESC");
		return EX_OSERR;
	}

	unsigned int pos = 0;
	do {
		unsigned short usage_page = 0;
		unsigned short usage = 0;

		ret = get_next_hid_usage(rpt_desc.value, rpt_desc.size, &pos, &usage_page, &usage);
		if (ret == 0) {
			if (usage_page == RAW_USAGE_PAGE && usage == RAW_USAGE_ID) {
				return EX_OK;
			}
		} else if (ret == -1) {
			return EX_DATAERR;
		}
	} while (ret != 0);

	return EX_UNAVAILABLE;
}

static int send_report(const std::string &device, int fd) {
	std::vector<uint8_t> data {
		/* Report ID */
		0x00,

		/* Command: Identify */
		0x00, 0x01,

		/* OS: Linux */
		0x1D, 0x6B,
	};

	ssize_t ret = ::write(fd, data.data(), data.size());
	if (ret < 0 || (size_t)ret != data.size()) {
		perror(device.c_str());
		return EX_IOERR;
	}

	return EX_OK;
}

int qmk_identify(const std::string &device) {
	int fd = ::open(device.c_str(), O_RDWR | O_NONBLOCK | O_CLOEXEC);
	int ret;

	if (fd == -1) {
		perror(device.c_str());
		ret = EX_NOINPUT;
		goto out;
	}

	ret = is_qmk_device(fd);
	if (ret != EX_OK) {
		if (ret == EX_UNAVAILABLE) {
			std::cerr << device << ": Not a QMK device" << std::endl;
		} else if (ret == EX_DATAERR) {
			std::cerr << device << ": Malformed report descriptor" << std::endl;
		}
		goto out;
	}

	ret = send_report(device, fd);

out:
	if (fd != -1) {
		::close(fd);
	}
	return ret;
}
