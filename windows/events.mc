;/*
;	qmk-hid-identify - Identify the current OS to QMK device
;	Copyright 2021-2022  Simon Arlott
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
; * Logging categories are a 16 bit value. Category 0 indicates that there is
; * is no category specified ("None") so it can't be used. The configuration
; * of a category count implies that these should be numbered consecutively
; * from 1 to allow the available values to be enumerated and labelled.
; *
; * The "Event Viewer" application accesses categories via the "Windows Event
; * Log" service which needs to have read access to the executable containing
; * the message table.
; */

MessageId=0x0001
SymbolicName=LOGGING_CATEGORY_REPORT_SENT_ID
Language=en_GB
Report sent
.

MessageId=0x0002
SymbolicName=LOGGING_CATEGORY_OS_ERROR_ID
Language=en_GB
OS Error
.

MessageId=0x0003
SymbolicName=LOGGING_CATEGORY_IO_ERROR_ID
Language=en_GB
I/O Error
.

MessageId=0x0004
SymbolicName=LOGGING_CATEGORY_UNSUPPORTED_DEVICE_ID
Language=en_GB
Unsupported device
.

MessageId=0x0005
SymbolicName=LOGGING_CATEGORY_SERVICE_ID
Language=en_GB
Service
.

;#define LOGGING_CATEGORY_MAX 0x0005

;/*
; * Severity and Facility are advisory only but log viewers will consider the
; * least significant 16 bits to be the "event ID" and the most significant
; * 16 bits to be the "qualifiers". Messages are separate for the full 32 bit
; * value. The event log type does not have to match the Severity.
; *
; * The "Event Viewer" application accesses messages directly, so the current
; * user needs to have read access to the executable containing the message
; * table.
; */

MessageId=0x0100
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_REPORT_SENT_ID
Language=en_GB
%1!s!: Report sent
.

MessageId=0x0110
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_NOT_ALLOWED_ID
Language=en_GB
%1!s!: Device not allowed
.

MessageId=0x0111
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_UNKNOWN_USAGE_ID
Language=en_GB
%1!s!: Not a QMK raw HID device interface
.

MessageId=0x0112
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_UNKNOWN_USB_INTERFACE_NUMBER_ID
Language=en_GB
%1!s!: Unknown USB interface number
.

MessageId=0x0113
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_NO_HID_ATTRIBUTES_ID
Language=en_GB
%1!s!: Unable to get HID attributes
.

MessageId=0x0114
Severity=Warning
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_ACCESS_DENIED_ID
Language=en_GB
%1!s!: Access denied
.

MessageId=0x0120
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_REPORT_COUNT_TOO_SMALL_ID
Language=en_GB
%1!s!: Report count too small for message (%2!s! < %3!s!)
.

MessageId=0x0121
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_REPORT_LENGTH_TOO_SMALL_ID
Language=en_GB
%1!s!: Report length too small for message (%2!s! < %3!s!)
.

MessageId=0x0130
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_WRITE_FAILED_ID
Language=en_GB
%1!s!: WriteFile: %2!s!
.

MessageId=0x0131
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_WRITE_TIMEOUT_ID
Language=en_GB
%1!s!: Report send timed out
.

MessageId=0x0132
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_SHORT_WRITE_ID
Language=en_GB
%1!s!: Write completed with only %2!s! of %3!s! bytes written
.

;#define LOGGING_MESSAGE_DEV_REPORT_DESCRIPTOR_SIZE_NEGATIVE_ID 0
;#define LOGGING_MESSAGE_DEV_REPORT_DESCRIPTOR_SIZE_TOO_LARGE_ID 0
;#define LOGGING_MESSAGE_DEV_MALFORMED_REPORT_DESCRIPTOR_ID 0

MessageId=0x1000
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_OS_FUNC_ERROR_CODE_1_ID
Language=en_GB
%1!s!: %2!s!: %3!s!
.

MessageId=0x1001
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_OS_FUNC_ERROR_CODE_2_ID
Language=en_GB
%1!s!: %2!s!: %3!s!, %4!s!
.

MessageId=0x1002
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_DEV_OS_FUNC_ERROR_PARAM_1_CODE_1_ID
Language=en_GB
%1!s!: %2!s!(%3!s!): %4!s!
.

MessageId=0x0200
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_STARTING_ID
Language=en_GB
Service starting
.

MessageId=0x0201
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_STARTED_ID
Language=en_GB
Service started
.

MessageId=0x0202
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_STOPPING_ID
Language=en_GB
Service stopping
.

MessageId=0x0203
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_STOPPED_ID
Language=en_GB
Service stopped
.

MessageId=0x0204
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_FAILED_ID
Language=en_GB
Service failed
.

MessageId=0x0210
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_MAIN_MUTEX_FAILURE_ID
Language=en_GB
Service main thread failed to acquire device queue mutex
.

MessageId=0x0211
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_CTRL_MUTEX_FAILURE_ID
Language=en_GB
Service control handler failed to acquire device queue mutex
.

MessageId=0x0300
Severity=Informational
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_POWER_RESUME_ID
Language=en_GB
Power resumed
.

MessageId=0x2000
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_OS_FUNC_ERROR_CODE_1_ID
Language=en_GB
%1!s!: %2!s!
.

MessageId=0x2001
Severity=Error
Facility=Application
SymbolicName=LOGGING_MESSAGE_SVC_OS_FUNC_ERROR_CODE_2_ID
Language=en_GB
%1!s!: %2!s!, %3!s!
.
