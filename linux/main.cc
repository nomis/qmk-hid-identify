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
#include <sysexits.h>

#include <iostream>
#include <string>

#include "hid-identify.h"
#include "../common/types.h"

using namespace hid_identify;

int main(int argc, char *argv[]) {
	int exit_ret = 0;

	if (argc < 2) {
		std::cout << "Usage: " << argv[0] << " <hidraw device>..." << std::endl;
		return EX_USAGE;
	}

	try {
		for (int i = 1; i < argc; i++) {
			try {
				LinuxHIDDevice(argv[i]).identify();
			} catch (const UnavailableDevice&) {
				exit_ret = exit_ret ? exit_ret : EX_NOINPUT;
			} catch (const MalformedHIDReportDescriptor&) {
				exit_ret = exit_ret ? exit_ret : EX_DATAERR;
			} catch (const OSError&) {
				exit_ret = exit_ret ? exit_ret : EX_OSERR;
			} catch (const IOError&) {
				exit_ret = exit_ret ? exit_ret : EX_IOERR;
			} catch (const UnsupportedDevice&) {
				exit_ret = exit_ret ? exit_ret : EX_UNAVAILABLE;
			} catch (const Exception&) {
				exit_ret = exit_ret ? exit_ret : EX_SOFTWARE;
			}
		}
	} catch (...) {
		throw;
	}

	return exit_ret;
}
