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

#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <vector>

#include "hid-enumerate.h"
#include "hid-identify.h"
#include "registry.h"
#include "service.h"
#include "service-control.h"
#include "../common/types.h"
#include "windows++.h"

using namespace hid_identify;

struct Command {
public:
	std::function<int()> function;
	bool elevate;
	std::wstring description;
};

static std::map<std::wstring, Command> commands;

static void usage(const std::wstring &name) {
	bool first = true;
	size_t max_length = 0;

	std::wcout << "Usage: " << name << " <";
	for (const auto& command : commands) {
		if (command.second.description.empty()) {
			continue;
		}

		if (!first) {
			std::wcout << "|";
		}
		first = false;
		std::wcout << command.first;

		if (max_length < command.first.length()) {
			max_length = command.first.length();
		}
	}
	std::wcout << ">" << std::endl << std::endl;

	std::wcout << "Commands:" << std::endl;
	for (const auto& command : commands) {
		if (command.second.description.empty()) {
			continue;
		}

		std::wcout << "  " << std::left << std::setw(max_length + 2)
			<< command.first << std::setw(0)
			<< command.second.description << std::endl;
	}
}

static int command_install() {
	service_install();
	return 0;
}

static int command_uninstall() {
	service_uninstall();
	return 0;
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

	for (const auto& device : WindowsHIDEnumeration()) {
		try {
			WindowsHIDDevice(device).identify();
		} catch (const Exception&) {
			exit_ret = exit_ret ? exit_ret : 1;
		} catch (const std::exception &e) {
			std::wcerr << e.what() << std::endl;
			exit_ret = exit_ret ? exit_ret : 1;
		}
	}

	return exit_ret;
}

int wmain(int argc, wchar_t *argv[], wchar_t *envp[] __attribute__((unused))) {
	commands = {
		{L"install", {command_install, true, L"Install and start service"}},
		{L"uninstall", {command_uninstall, true, L"Stop and uninstall service"}},

		{L"register", {command_register, true, L"Add event source to registry"}},
		{L"unregister", {command_unregister, true, L"Remove event source from registry"}},

		{L"report", {command_report, false, L"Send HID report to all devices"}},
		{L"service", {command_service, false, L""}},
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
	} catch (const std::exception &e) {
		std::wcerr << e.what() << std::endl;
		return 2;
	} catch (...) {
		throw;
	}
}
