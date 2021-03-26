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
#include "usb-vid-pid.h"

#include <stdbool.h>
#include <stdint.h>

#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))

struct usb_device {
	uint16_t vid;
	uint16_t pid;
	uint16_t pid_mask;
};

static const struct usb_device devices[] = {
	{ 0x1209, 0x0000, 0xF000 },
	{ 0x16C0, 0x05DF, 0xFFFF },
	{ 0x16C0, 0x27D9, 0xFFFF },
	{ 0x16C0, 0x27DA, 0xFFFE },
	{ 0x16C0, 0x27DC, 0xFFFF },
};

bool usb_device_allowed(uint16_t vid, uint16_t pid) {
	for (unsigned int i = 0; i < ARRAY_SIZE(devices); i++) {
		if (vid == devices[i].vid && (pid & devices[i].pid_mask) == devices[i].pid) {
			return true;
		}
	}

	return false;
}
