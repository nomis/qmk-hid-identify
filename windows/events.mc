;/*
;	qmk-hid-identify - Identify the current OS to QMK device
;	Copyright 2021  Simon Arlott
;
;	This program is free software: you can redistribute it and/or modify
;	it under the terms of the GNU General Public License as published by
;	the Free Software Foundation, either version 3 of the License, or
;	(at your option) any later version.
;
;	This program is distributed in the hope that it will be useful,
;	but WITHOUT ANY WARRANTY; without even the implied warranty of
;	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;	GNU General Public License for more details.
;
;	You should have received a copy of the GNU General Public License
;	along with this program.  If not, see <https://www.gnu.org/licenses/>.
;*/
;#pragma once

LanguageNames=(
	en_GB=0x0809:en_GB
)

;/*
; * The "Event Viewer" application accesses categories via the "Windows Event
; * Log" service which needs to have read access to the executable containing
; * the message table.
; *
; * Using System.Diagnostics.EventLog in PowerShell will access categories
; * directly as the current user.
; *
; * https://stackoverflow.com/a/48427494
; */

MessageId=0x01
SymbolicName=LOGGING_CATEGORY_REPORT_SENT_ID
Language=en_GB
Report sent
.

MessageId=0x02
SymbolicName=LOGGING_CATEGORY_OS_ERROR_ID
Language=en_GB
OS Error
.

MessageId=0x03
SymbolicName=LOGGING_CATEGORY_IO_ERROR_ID
Language=en_GB
I/O Error
.

MessageId=0x04
SymbolicName=LOGGING_CATEGORY_UNSUPPORTED_DEVICE_ID
Language=en_GB
Unsupported device
.

MessageId=0x05
SymbolicName=LOGGING_CATEGORY_SERVICE_ID
Language=en_GB
Service
.

;#define LOGGING_CATEGORY_MAX 0x05

;/*
; * The "Event Viewer" application accesses messages directly, so the current
; * user needs to have read access to the executable containing the message
; * table.
; */

MessageId=0x100
SymbolicName=LOGGING_MESSAGE_DEV_REPORT_SENT_ID
Language=en_GB
%1!s!: Report sent
.

MessageId=0x110
SymbolicName=LOGGING_MESSAGE_DEV_NOT_ALLOWED_ID
Language=en_GB
%1!s!: Device not allowed
.

MessageId=0x111
SymbolicName=LOGGING_MESSAGE_DEV_UNKNOWN_USAGE_ID
Language=en_GB
%1!s!: Not a QMK raw HID device interface
.

MessageId=0x112
SymbolicName=LOGGING_MESSAGE_DEV_UNKNOWN_USB_INTERFACE_NUMBER_ID
Language=en_GB
%1!s!: Unknown USB interface number
.

MessageId=0x113
SymbolicName=LOGGING_MESSAGE_DEV_NO_HID_ATTRIBUTES_ID
Language=en_GB
%1!s!: Unable to get HID attributes
.

MessageId=0x114
SymbolicName=LOGGING_MESSAGE_DEV_ACCESS_DENIED_ID
Language=en_GB
%1!s!: Access denied
.

MessageId=0x120
SymbolicName=LOGGING_MESSAGE_DEV_REPORT_COUNT_TOO_SMALL_ID
Language=en_GB
%1!s!: Report count too small for message (%2!s! < %3!s!)
.

MessageId=0x121
SymbolicName=LOGGING_MESSAGE_DEV_REPORT_LENGTH_TOO_SMALL_ID
Language=en_GB
%1!s!: Report length too small for message (%2!s! < %3!s!)
.

MessageId=0x130
SymbolicName=LOGGING_MESSAGE_DEV_WRITE_FAILED_ID
Language=en_GB
%1!s!: WriteFile: %2!s!
.

MessageId=0x131
SymbolicName=LOGGING_MESSAGE_DEV_WRITE_TIMEOUT_ID
Language=en_GB
%1!s!: Report send timed out
.

MessageId=0x132
SymbolicName=LOGGING_MESSAGE_DEV_SHORT_WRITE_ID
Language=en_GB
%1!s!: Write completed with only %2!s! of %3!s! bytes written
.

;#define LOGGING_MESSAGE_DEV_REPORT_DESCRIPTOR_SIZE_NEGATIVE_ID 0
;#define LOGGING_MESSAGE_DEV_REPORT_DESCRIPTOR_SIZE_TOO_LARGE_ID 0
;#define LOGGING_MESSAGE_DEV_MALFORMED_REPORT_DESCRIPTOR_ID 0

MessageId=0x1000
SymbolicName=LOGGING_MESSAGE_DEV_OS_FUNC_ERROR_CODE_1_ID
Language=en_GB
%1!s!: %2!s!: %3!s!
.

MessageId=0x1001
SymbolicName=LOGGING_MESSAGE_DEV_OS_FUNC_ERROR_CODE_2_ID
Language=en_GB
%1!s!: %2!s!: %3!s!, %4!s!
.

MessageId=0x1002
SymbolicName=LOGGING_MESSAGE_DEV_OS_FUNC_ERROR_PARAM_1_CODE_1_ID
Language=en_GB
%1!s!: %2!s!(%3!s!): %4!s!
.
