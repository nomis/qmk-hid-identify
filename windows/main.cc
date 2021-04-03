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
#include <windows.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include <functional>
#include <iomanip>
#include <map>
#include <vector>

#include "hid-enumerate.h"
#include "hid-identify.h"
#include "registry.h"
#include "../common/types.h"
#include "windows++.h"

using namespace hid_identify;

struct Command {
public:
	std::function<int()> function;
	bool elevate;
	win32::native_string description;
};

static std::map<win32::native_string, Command> commands;

static void usage(const win32::native_string &name) {
	bool first = true;
	size_t max_length = 0;

	win32::cout << "Usage: " << name << " <";
	for (const auto& command : commands) {
		if (command.second.description.empty()) {
			continue;
		}

		if (!first) {
			win32::cout << "|";
		}
		first = false;
		win32::cout << command.first;

		if (max_length < command.first.length()) {
			max_length = command.first.length();
		}
	}
	win32::cout << ">" << std::endl << std::endl;

	win32::cout << "Commands:" << std::endl;
	for (const auto& command : commands) {
		if (command.second.description.empty()) {
			continue;
		}

		win32::cout << "  " << std::left << std::setw(max_length + 2)
			<< command.first << std::setw(0)
			<< command.second.description << std::endl;
	}
}

static int command_install() {
	return 1;
}

static int command_uninstall() {
	return 1;
}

static int command_register() {
	registry_add_event_log();
	return 0;
}

static int command_unregister() {
	registry_remove_event_log();
	return 0;
}

static int command_report() {
	int exit_ret = 0;

	for (auto& device : enumerate_devices()) {
		try {
			WindowsHIDDevice(device).identify();
		} catch (const Exception&) {
			exit_ret = exit_ret ? exit_ret : 1;
		}
	}

	return exit_ret;
}

static int command_service() {
	return 1;
}

int
#ifdef UNICODE
wmain
#else
main
#endif
(int argc, win32::native_char *argv[], win32::native_char *envp[] __attribute__((unused))) {
	commands = {
		{TEXT("install"), {command_install, true, TEXT("Install and start service")}},
		{TEXT("uninstall"), {command_uninstall, true, TEXT("Stop and uninstall service")}},

		{TEXT("register"), {command_register, true, TEXT("Add event source to registry")}},
		{TEXT("unregister"), {command_unregister, true, TEXT("Remove event source from registry")}},

		{TEXT("report"), {command_report, false, TEXT("Send HID report to all devices")}},
		{TEXT("service"), {command_service, false, TEXT("")}},
	};

	if (argc != 2) {
		usage(argv[0]);
		return 1;
	}

	auto command = commands.find(argv[1]);

	if (command == commands.end()) {
		usage(argv[0]);
		return 1;
	}

	try {
		if (command->second.elevate) {
			if (!win32::is_elevated()) {
				return win32::run_elevated({command->first});
			}
		}

		return command->second.function();
	} catch (const Exception&) {
		return 1;
	} catch (...) {
		throw;
	}
}
