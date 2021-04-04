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

#include <windows.h>

#ifndef NOGDI
#	undef ERROR
#endif

#include <vector>

static inline const char *gettext(const char *msgid) { return msgid; }

#define LOGGING_HAS_LEVEL_IDS

#define LOGGING_LEVEL_ERROR_ID EVENTLOG_ERROR_TYPE
#define LOGGING_LEVEL_WARNING_ID EVENTLOG_WARNING_TYPE
#define LOGGING_LEVEL_INFO_ID EVENTLOG_INFORMATION_TYPE

#define LOGGING_HAS_CATEGORY_IDS

#define LOGGING_HAS_MESSAGE_IDS

#include "events.h"
